using System.IO.Pipes;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;

namespace IgniteBT.Daemon;

/// <summary>
/// IPC client for the persistent IgniteBT daemon.
/// </summary>
public static class DaemonClient
{
    private static readonly JsonSerializerOptions JsonOptions = new() { PropertyNamingPolicy = JsonNamingPolicy.CamelCase };

    public static bool TryExecute(
        string projectRoot,
        string command,
        string[] args,
        string workingDirectory,
        out DaemonResponse? response,
        int connectTimeoutMs = 100)
    {
        response = null;
        try
        {
            if (OperatingSystem.IsWindows())
                return TryExecuteNamedPipe(projectRoot, command, args, workingDirectory, out response, connectTimeoutMs);

            return TryExecuteUnixSocket(projectRoot, command, args, workingDirectory, out response, connectTimeoutMs);
        }
        catch
        {
            return false;
        }
    }

    private static bool TryExecuteNamedPipe(
        string projectRoot,
        string command,
        string[] args,
        string workingDirectory,
        out DaemonResponse? response,
        int connectTimeoutMs)
    {
        response = null;
        var pipeName = DaemonPaths.GetPipeName(projectRoot);

        using var pipe = new NamedPipeClientStream(".", pipeName, PipeDirection.InOut, PipeOptions.None);
        try
        {
            pipe.Connect(connectTimeoutMs);
        }
        catch (TimeoutException)
        {
            return false;
        }

        return Exchange(pipe, command, args, workingDirectory, out response);
    }

    private static bool TryExecuteUnixSocket(
        string projectRoot,
        string command,
        string[] args,
        string workingDirectory,
        out DaemonResponse? response,
        int connectTimeoutMs)
    {
        response = null;
        var socketPath = DaemonPaths.GetPipeName(projectRoot);
        if (!File.Exists(socketPath)) return false;

        using var socket = new Socket(AddressFamily.Unix, SocketType.Stream, ProtocolType.Unspecified);
        try
        {
            socket.Connect(new UnixDomainSocketEndPoint(socketPath));
        }
        catch
        {
            return false;
        }

        using var stream = new NetworkStream(socket, ownsSocket: false);
        return Exchange(stream, command, args, workingDirectory, out response);
    }

    private static bool Exchange(Stream stream, string command, string[] args, string workingDirectory, out DaemonResponse? response)
    {
        response = null;
        var request = JsonSerializer.Serialize(new DaemonRequest
        {
            Command = command,
            Args = args,
            WorkingDirectory = workingDirectory
        }, JsonOptions);

        var requestBytes = Encoding.UTF8.GetBytes(request);
        var lengthBytes = BitConverter.GetBytes(requestBytes.Length);
        stream.Write(lengthBytes);
        stream.Write(requestBytes);
        stream.Flush();

        var lenBuf = new byte[4];
        if (ReadExact(stream, lenBuf, 4) != 4) return false;
        var responseLen = BitConverter.ToInt32(lenBuf);
        if (responseLen <= 0 || responseLen > 16 * 1024 * 1024) return false;

        var responseBytes = new byte[responseLen];
        if (ReadExact(stream, responseBytes, responseLen) != responseLen) return false;

        response = DaemonResponse.Deserialize(Encoding.UTF8.GetString(responseBytes));
        return response != null;
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
}
