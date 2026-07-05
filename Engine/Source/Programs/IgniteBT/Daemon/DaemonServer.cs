using System.Diagnostics;
using System.IO.Pipes;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;
using Serilog;
using IgniteBT.CLI;

namespace IgniteBT.Daemon;

/// <summary>
/// Persistent IPC server — warm compiler pool, caches, and sub-ms no-op responses.
/// </summary>
public sealed class DaemonServer : IDisposable
{
    private readonly string _projectRoot;
    private readonly IgniteBTDaemon _daemon;
    private readonly CancellationTokenSource _cts = new();
    private Task? _acceptLoop;
    private Socket? _unixSocket;

    public DaemonServer(string projectRoot)
    {
        _projectRoot = projectRoot;
        _daemon = new IgniteBTDaemon(projectRoot);
    }

    public void Start()
    {
        Directory.CreateDirectory(Path.Combine(_projectRoot, "Build", "Temp"));
        File.WriteAllText(DaemonPaths.GetPidFilePath(_projectRoot), Environment.ProcessId.ToString());
        _daemon.Start();

        if (OperatingSystem.IsWindows())
            _acceptLoop = Task.Run(AcceptNamedPipesAsync);
        else
            _acceptLoop = Task.Run(AcceptUnixSocketsAsync);

        Log.Information("Daemon IPC listening on {Endpoint}", DaemonPaths.GetPipeName(_projectRoot));
    }

    private async Task AcceptNamedPipesAsync()
    {
        var pipeName = DaemonPaths.GetPipeName(_projectRoot);
        while (!_cts.IsCancellationRequested)
        {
            var pipe = new NamedPipeServerStream(
                pipeName,
                PipeDirection.InOut,
                NamedPipeServerStream.MaxAllowedServerInstances,
                PipeTransmissionMode.Byte,
                PipeOptions.Asynchronous);

            try
            {
                await pipe.WaitForConnectionAsync(_cts.Token);
                _ = Task.Run(() => HandleClient(pipe));
            }
            catch (OperationCanceledException)
            {
                pipe.Dispose();
                break;
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Daemon pipe accept failed");
                pipe.Dispose();
            }
        }
    }

    private async Task AcceptUnixSocketsAsync()
    {
        var socketPath = DaemonPaths.GetPipeName(_projectRoot);
        if (File.Exists(socketPath)) File.Delete(socketPath);

        _unixSocket = new Socket(AddressFamily.Unix, SocketType.Stream, ProtocolType.Unspecified);
        _unixSocket.Bind(new UnixDomainSocketEndPoint(socketPath));
        _unixSocket.Listen(16);

        while (!_cts.IsCancellationRequested)
        {
            try
            {
                var client = await _unixSocket.AcceptAsync(_cts.Token);
                _ = Task.Run(() => HandleUnixClient(client));
            }
            catch (OperationCanceledException)
            {
                break;
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Daemon socket accept failed");
            }
        }
    }

    private void HandleUnixClient(Socket client)
    {
        using (client)
        using (var stream = new NetworkStream(client, ownsSocket: false))
            HandleClient(stream);
    }

    private void HandleClient(Stream stream)
    {
        using (stream)
        {
            try
            {
                var lenBuf = new byte[4];
                if (ReadExact(stream, lenBuf, 4) != 4) return;
                var requestLen = BitConverter.ToInt32(lenBuf);
                if (requestLen <= 0 || requestLen > 1024 * 1024) return;

                var requestBytes = new byte[requestLen];
                if (ReadExact(stream, requestBytes, requestLen) != requestLen) return;

                var request = JsonSerializer.Deserialize<DaemonRequest>(
                    Encoding.UTF8.GetString(requestBytes));
                if (request == null) return;

                var response = ProcessRequest(request);
                var responseBytes = Encoding.UTF8.GetBytes(DaemonResponse.Serialize(response));
                var lengthBytes = BitConverter.GetBytes(responseBytes.Length);
                stream.Write(lengthBytes);
                stream.Write(responseBytes);
                stream.Flush();
            }
            catch (Exception ex)
            {
                Log.Debug(ex, "Daemon client handler error");
            }
        }
    }

    private DaemonResponse ProcessRequest(DaemonRequest request)
    {
        var sw = Stopwatch.StartNew();
        var previousDir = Directory.GetCurrentDirectory();
        try
        {
            if (!string.IsNullOrEmpty(request.WorkingDirectory)
                && Directory.Exists(request.WorkingDirectory))
            {
                Directory.SetCurrentDirectory(request.WorkingDirectory);
            }

            return request.Command.ToLowerInvariant() switch
            {
                "ping" => new DaemonResponse { ExitCode = 0, Output = "pong", ElapsedMs = sw.ElapsedMilliseconds },
                "build" => ExecuteBuild(request.Args, sw),
                _ => new DaemonResponse { ExitCode = 1, Error = $"Unknown command: {request.Command}", ElapsedMs = sw.ElapsedMilliseconds }
            };
        }
        finally
        {
            Directory.SetCurrentDirectory(previousDir);
        }
    }

    private DaemonResponse ExecuteBuild(string[] args, Stopwatch sw)
    {
        var fast = BuildCommand.TryFastNoOp(args);
        if (fast.HasValue)
        {
            sw.Stop();
            return new DaemonResponse
            {
                ExitCode = fast.Value,
                Output = "No-op build - nothing changed (daemon)",
                WasNoOp = true,
                ElapsedMs = sw.ElapsedMilliseconds
            };
        }

        try
        {
            var exitCode = BuildCommand.Execute(args).GetAwaiter().GetResult();
            sw.Stop();
            return new DaemonResponse { ExitCode = exitCode, ElapsedMs = sw.ElapsedMilliseconds };
        }
        catch (Exception ex)
        {
            sw.Stop();
            return new DaemonResponse { ExitCode = 1, Error = ex.Message, ElapsedMs = sw.ElapsedMilliseconds };
        }
    }

    private static int ReadExact(Stream stream, byte[] buffer, int count)
    {
        var read = 0;
        while (read < count)
        {
            var n = stream.Read(buffer, read, count - read);
            if (n == 0) return read;
            read += n;
        }
        return read;
    }

    public void Dispose()
    {
        _cts.Cancel();
        try { _acceptLoop?.Wait(TimeSpan.FromSeconds(2)); } catch { /* best effort */ }
        _unixSocket?.Dispose();
        _daemon.Dispose();
        try { File.Delete(DaemonPaths.GetPidFilePath(_projectRoot)); } catch { /* best effort */ }
        _cts.Dispose();
    }
}
