#include "Renderer/AtmosphereRadianceTracer.hpp"

#include "Core/LogCategory.hpp"
#include "Core/DiagnosticMacros.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace we::runtime::renderer {

#if WE_HAS_GLM

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kRayleighScaleKm = 8.0f;
constexpr float kMieScaleKm = 1.2f;
constexpr float kOzonePeakAltKm = 25.0f;
constexpr float kOzoneWidthKm = 8.0f;
constexpr float kSunAngularRadius = 0.004675f;
constexpr int kAtmosphereSteps = 32;
constexpr int kSunTransmittanceSteps = 16;
constexpr float kSkyRadianceScale = 6.0f;
constexpr float kMaxExpectedHdr = 1000.0f;

struct AtmosParams {
    glm::vec3 rayleigh;
    float mie;
    glm::vec3 ozone;
    float mieG;
    float planetR;
    float atmoR;
    float multiScatter;
    float eyeAltitude;
    glm::vec3 sunColor;
    float sunIntensity;
    float sunAngularRadius;
    glm::vec3 sunDir;
};

AtmosParams BuildParams(const SceneEnvironmentUniform& env) {
    AtmosParams p{};
    p.rayleigh = glm::max(env.atmosphereRayleigh, glm::vec3(1e-6f));
    p.mie = std::max(env.mieScattering, 1e-6f);
    p.ozone = glm::max(env.ozoneAbsorption, glm::vec3(0.0f));
    p.mieG = env.mieAnisotropy;
    p.planetR = std::max(env.planetRadius, 1.0f);
    p.atmoR = p.planetR + std::max(env.atmosphereHeight, 0.1f);
    p.multiScatter = std::max(env.multiScatterStrength, 0.0f);
    p.eyeAltitude = std::max(env.eyeAltitude, 0.001f);
    p.sunColor = glm::max(glm::vec3(env.sunColor), glm::vec3(0.0f));
    p.sunIntensity = std::max(env.sunIntensity, 0.0f);
    p.sunAngularRadius = std::max(env.sunAngularRadius, kSunAngularRadius);
    p.sunDir = glm::normalize(-glm::vec3(env.sunDirection));
    return p;
}

glm::vec3 UnprojectDirection(const glm::vec2& uv, const glm::mat4& view, const glm::mat4& proj, const glm::vec3& cameraPos) {
    const glm::vec2 ndc = uv * 2.0f - glm::vec2(1.0f);
    const glm::mat4 invView = glm::inverse(view);
    const glm::mat4 invProj = glm::inverse(proj);
    const glm::vec4 clip(ndc.x, ndc.y, 0.0f, 1.0f);
    const glm::vec4 world = invView * invProj * clip;
    const glm::vec3 farPoint = glm::vec3(world) / std::max(world.w, 1e-6f);
    return glm::normalize(farPoint - cameraPos);
}

glm::vec3 GetAtmosphereOrigin(const glm::vec3& cameraPos, const glm::vec3& worldOrigin, float planetRadiusKm, float eyeAltitudeKm) {
    const glm::vec3 relKm = (cameraPos - worldOrigin) * 0.001f;
    const float altitudeKm = std::max(std::max(relKm.y, 0.0f), std::max(eyeAltitudeKm, 0.0f));
    return glm::vec3(0.0f, planetRadiusKm + altitudeKm, 0.0f);
}

glm::vec2 LutTransmittanceParamsToUv(float viewHeightKm, float viewZenithCosAngle, const AtmosParams& p) {
    const float bottomR = p.planetR;
    const float topR = p.atmoR;
    const float H = std::sqrt(std::max(topR * topR - bottomR * bottomR, 0.0f));
    const float rho = std::sqrt(std::max(viewHeightKm * viewHeightKm - bottomR * bottomR, 0.0f));

    const float discriminant = viewHeightKm * viewHeightKm * (viewZenithCosAngle * viewZenithCosAngle - 1.0f)
        + topR * topR;
    const float d = std::max(0.0f, (-viewHeightKm * viewZenithCosAngle + std::sqrt(std::max(discriminant, 0.0f))));

    const float dMin = topR - viewHeightKm;
    const float dMax = rho + H;
    const float xMu = (d - dMin) / std::max(dMax - dMin, 1e-4f);
    const float xR = rho / std::max(H, 1e-4f);
    return glm::vec2(std::clamp(xMu, 0.0f, 1.0f), std::clamp(xR, 0.0f, 1.0f));
}

glm::vec2 SkyViewLUTCoord(const glm::vec3& viewDir, const glm::vec3& sunDir, float& outZenithAngle) {
    const glm::vec3 vd = glm::normalize(viewDir);
    const glm::vec3 sd = glm::normalize(sunDir);
    outZenithAngle = std::acos(std::clamp(vd.y, -1.0f, 1.0f)) / kPi;

    const float sunAzimuth = std::atan2(sd.z, sd.x);
    const float viewAzimuth = std::atan2(vd.z, vd.x);
    float azimuthOffset = (viewAzimuth - sunAzimuth) / (2.0f * kPi);
    azimuthOffset -= std::floor(azimuthOffset);
    return glm::vec2(azimuthOffset, outZenithAngle);
}

float SunDiskMask(const glm::vec3& viewDir, const glm::vec3& sunDir, float angularRadius) {
    const float cosAngle = glm::dot(glm::normalize(viewDir), glm::normalize(sunDir));
    const float cosRadius = std::cos(std::max(angularRadius, kSunAngularRadius));
    const float t = std::clamp((cosAngle - (cosRadius - 0.00015f)) / 0.00015f, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float AtmosphereDensity(float heightKm, float scaleHeightKm) {
    return std::exp(-std::max(heightKm, 0.0f) / std::max(scaleHeightKm, 1e-3f));
}

float OzoneDensity(float heightKm) {
    const float x = (heightKm - kOzonePeakAltKm) / kOzoneWidthKm;
    return std::exp(-x * x);
}

bool IntersectSphere(const glm::vec3& origin, const glm::vec3& dir, float radius, float& t0, float& t1) {
    const float b = glm::dot(origin, dir);
    const float c = glm::dot(origin, origin) - radius * radius;
    const float discriminant = b * b - c;
    if (discriminant < 0.0f) {
        return false;
    }
    const float s = std::sqrt(discriminant);
    t0 = -b - s;
    t1 = -b + s;
    return t1 > 0.0f;
}

glm::vec3 IntegrateOpticalDepth(
    const glm::vec3& origin,
    const glm::vec3& dir,
    float maxDist,
    const AtmosParams& p) {
    glm::vec3 rayleighDepth(0.0f);
    glm::vec3 mieDepth(0.0f);
    glm::vec3 ozoneDepth(0.0f);

    const float stepSize = maxDist / static_cast<float>(kSunTransmittanceSteps);
    glm::vec3 marchPos = origin;

    for (int i = 0; i < kSunTransmittanceSteps; ++i) {
        const float heightKm = std::max(glm::length(marchPos) - p.planetR, 0.0f);
        rayleighDepth += p.rayleigh * AtmosphereDensity(heightKm, kRayleighScaleKm) * stepSize;
        mieDepth += glm::vec3(p.mie) * AtmosphereDensity(heightKm, kMieScaleKm) * stepSize;
        ozoneDepth += p.ozone * OzoneDensity(heightKm) * stepSize;
        marchPos += dir * stepSize;
    }

    return glm::exp(-(rayleighDepth + mieDepth + ozoneDepth));
}

glm::vec3 IntegrateSunTransmittance(const glm::vec3& samplePosRel, const glm::vec3& sunDir, const AtmosParams& p) {
    float t0 = 0.0f;
    float t1 = 0.0f;
    if (!IntersectSphere(samplePosRel, sunDir, p.atmoR, t0, t1)) {
        return glm::vec3(1.0f);
    }
    const float dist = (t0 > 0.0f) ? t0 : std::max(t1, 0.001f);
    return IntegrateOpticalDepth(samplePosRel, sunDir, dist, p);
}

float RayleighPhase(float cosTheta) {
    return (3.0f / (16.0f * kPi)) * (1.0f + cosTheta * cosTheta);
}

float MiePhase(float cosTheta, float g) {
    const float g2 = g * g;
    const float num = (1.0f - g2);
    const float denom = std::pow(std::max(1.0f + g2 - 2.0f * g * cosTheta, 1e-4f), 1.5f);
    return (3.0f / (8.0f * kPi)) * ((1.0f + g2) * num / denom);
}

struct InscatterResult {
    glm::vec3 skyRadiance;
    glm::vec3 rayleigh;
    glm::vec3 mie;
    glm::vec3 multiScatter;
    glm::vec3 transmittance;
};

InscatterResult IntegrateInscatteringDetailed(
    const glm::vec3& viewDir,
    const glm::vec3& sunDir,
    const glm::vec3& origin,
    const AtmosParams& p) {
    InscatterResult result{};
    result.transmittance = glm::vec3(1.0f);

    float t0 = 0.0f;
    float t1 = 0.0f;
    if (!IntersectSphere(origin, viewDir, p.atmoR, t0, t1)) {
        return result;
    }

    const float start = std::max(t0, 0.0f);
    const float end = t1;
    const float stepSize = (end - start) / static_cast<float>(kAtmosphereSteps);

    glm::vec3 sumRayleigh(0.0f);
    glm::vec3 sumMie(0.0f);
    glm::vec3 opticalRayleigh(0.0f);
    glm::vec3 opticalMie(0.0f);
    glm::vec3 opticalOzone(0.0f);

    const float cosTheta = glm::dot(viewDir, sunDir);
    const float phaseR = RayleighPhase(cosTheta);
    const float phaseM = MiePhase(cosTheta, p.mieG);

    for (int i = 0; i < kAtmosphereSteps; ++i) {
        const float t = start + (static_cast<float>(i) + 0.5f) * stepSize;
        const glm::vec3 samplePos = origin + viewDir * t;
        const float heightKm = std::max(glm::length(samplePos) - p.planetR, 0.0f);

        const float rd = AtmosphereDensity(heightKm, kRayleighScaleKm);
        const float md = AtmosphereDensity(heightKm, kMieScaleKm);
        const float od = OzoneDensity(heightKm);

        opticalRayleigh += p.rayleigh * rd * stepSize;
        opticalMie += glm::vec3(p.mie) * md * stepSize;
        opticalOzone += p.ozone * od * stepSize;

        const glm::vec3 sunT = IntegrateSunTransmittance(samplePos, sunDir, p);
        const glm::vec3 tau = opticalRayleigh + opticalMie + opticalOzone;
        const glm::vec3 atten = glm::exp(-tau) * sunT;

        sumRayleigh += atten * rd * stepSize;
        sumMie += atten * md * stepSize;
    }

    result.transmittance = glm::exp(-(opticalRayleigh + opticalMie + opticalOzone));
    const glm::vec3 tauView = opticalRayleigh + opticalMie;
    const glm::vec3 multiScatter = (glm::vec3(1.0f) - glm::exp(-tauView * 0.08f))
        * p.rayleigh * p.multiScatter;

    const glm::vec3 sunRadiance = p.sunColor * p.sunIntensity;
    result.rayleigh = kSkyRadianceScale * sunRadiance * sumRayleigh * phaseR * p.rayleigh;
    result.mie = kSkyRadianceScale * sunRadiance * sumMie * phaseM * p.mie;
    result.multiScatter = kSkyRadianceScale * sunRadiance * multiScatter;
    result.skyRadiance = result.rayleigh + result.mie + result.multiScatter;
    return result;
}

glm::vec3 ComputeSunDisk(const glm::vec3& viewDir, const glm::vec3& sunDir, const AtmosParams& p) {
    const float disk = SunDiskMask(viewDir, sunDir, p.sunAngularRadius);
    return p.sunColor * p.sunIntensity * kSkyRadianceScale * disk;
}

std::string Vec3ToString(const glm::vec3& v) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6) << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return ss.str();
}

std::string Vec2ToString(const glm::vec2& v) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6) << "(" << v.x << ", " << v.y << ")";
    return ss.str();
}

bool IsFiniteVec3(const glm::vec3& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

} // namespace

AtmosphereRadianceTracer& AtmosphereRadianceTracer::Get() {
    static AtmosphereRadianceTracer instance;
    return instance;
}

AtmosphereRadianceStageSample AtmosphereRadianceTracer::TraceSample(
    const std::string& label,
    const glm::vec2& screenUV,
    const glm::mat4& view,
    const glm::mat4& proj,
    const glm::vec3& cameraPos,
    const SceneEnvironmentUniform& env) const {

    AtmosphereRadianceStageSample sample{};
    sample.label = label;
    sample.screenUV = screenUV;

    const AtmosParams params = BuildParams(env);
    const glm::vec3 viewDirRaw = UnprojectDirection(screenUV, view, proj, cameraPos);
    sample.viewDirectionLength = glm::length(viewDirRaw);
    sample.viewDirection = viewDirRaw / std::max(sample.viewDirectionLength, 1e-6f);
    sample.sunDirection = params.sunDir;
    sample.sunDirectionLength = glm::length(glm::vec3(env.sunDirection));
    sample.viewSunDot = glm::dot(sample.viewDirection, sample.sunDirection);

    const glm::vec3 origin = GetAtmosphereOrigin(cameraPos, env.worldOrigin, params.planetR, params.eyeAltitude);
    sample.atmosphereHeightKm = std::max(glm::length(origin) - params.planetR, 0.0f);
    const float viewHeightKm = glm::length(origin);

    float zenithAngle = 0.0f;
    sample.skyViewUV = SkyViewLUTCoord(sample.viewDirection, sample.sunDirection, zenithAngle);
    sample.transmittanceUV = LutTransmittanceParamsToUv(viewHeightKm, sample.viewDirection.y, params);

    const InscatterResult inscatter = IntegrateInscatteringDetailed(sample.viewDirection, sample.sunDirection, origin, params);
    sample.skyViewRGB = inscatter.skyRadiance;
    sample.rayleighRGB = inscatter.rayleigh;
    sample.mieRGB = inscatter.mie;
    sample.multiScatterRGB = inscatter.multiScatter;
    sample.transmittanceRGB = inscatter.transmittance;

    sample.sunDiskMask = SunDiskMask(sample.viewDirection, sample.sunDirection, params.sunAngularRadius);
    sample.sunDiskRGB = ComputeSunDisk(sample.viewDirection, sample.sunDirection, params);
    sample.finalHdrRGB = sample.skyViewRGB + sample.sunDiskRGB;
    sample.valid = true;
    return sample;
}

std::vector<AtmosphereRadianceStageSample> AtmosphereRadianceTracer::TraceStandardSamples(
    const glm::mat4& view,
    const glm::mat4& proj,
    const glm::vec3& cameraPos,
    const glm::vec3& cameraForward,
    const glm::vec3& cameraRight,
    const glm::vec3& cameraUp,
    const SceneEnvironmentUniform& env) const {

    std::vector<AtmosphereRadianceStageSample> samples;
    samples.push_back(TraceSample("center", glm::vec2(0.5f, 0.5f), view, proj, cameraPos, env));
    samples.push_back(TraceSample("elev45_screen", glm::vec2(0.5f, 0.25f), view, proj, cameraPos, env));
    samples.push_back(TraceSample("horizon_screen", glm::vec2(0.5f, 0.92f), view, proj, cameraPos, env));
    return samples;
}

bool AtmosphereRadianceTracer::ValidateSample(
    const AtmosphereRadianceStageSample& sample,
    AtmosphereRadianceValidationFailure& outFailure) const {

    auto fail = [&](const char* stage, const char* function, const char* file, int line,
                    const char* variable, const std::string& previous, const std::string& invalid, const char* op) {
        outFailure.stage = stage;
        outFailure.function = function;
        outFailure.sourceFile = file;
        outFailure.sourceLine = line;
        outFailure.variable = variable;
        outFailure.previousValid = previous;
        outFailure.invalidValue = invalid;
        outFailure.operation = op;
        return false;
    };

    if (!sample.valid) {
        return fail("init", "TraceSample", __FILE__, __LINE__, "valid", "true", "false", "sample construction");
    }

    if (std::abs(sample.viewDirectionLength - 1.0f) > 0.01f) {
        return fail("viewDirection", "UnprojectDirection", "Math.hlsli", 83, "length(viewDir)",
            "1.0", std::to_string(sample.viewDirectionLength), "normalize(WE_UnprojectDirection(...))");
    }

    if (std::abs(sample.sunDirectionLength - 1.0f) > 0.01f) {
        return fail("sunDirection", "BuildParams", __FILE__, __LINE__, "length(sunDir)",
            "1.0", std::to_string(sample.sunDirectionLength), "normalize(-sunDirection)");
    }

    if (sample.viewSunDot < -1.01f || sample.viewSunDot > 1.01f) {
        return fail("viewSunDot", "dot", __FILE__, __LINE__, "dot(view,sun)",
            "[-1,1]", std::to_string(sample.viewSunDot), "dot(normalized view, normalized sun)");
    }

    if (sample.skyViewUV.x < -0.01f || sample.skyViewUV.x > 1.01f || sample.skyViewUV.y < -0.01f || sample.skyViewUV.y > 1.01f) {
        return fail("skyViewUV", "WE_SkyViewLUTCoord", "AtmosphereCommon.hlsli", 134, "skyViewUV",
            "[0,1]", Vec2ToString(sample.skyViewUV), "sun-relative azimuth/zenith mapping");
    }

    if (sample.transmittanceUV.x < -0.01f || sample.transmittanceUV.x > 1.01f
        || sample.transmittanceUV.y < -0.01f || sample.transmittanceUV.y > 1.01f) {
        return fail("transmittanceUV", "WE_LutTransmittanceParamsToUv", "AtmosphereCommon.hlsli", 115, "transmittanceUV",
            "[0,1]", Vec2ToString(sample.transmittanceUV), "UE5 transmittance parameterization");
    }

    const auto checkRgb = [&](const char* stage, const char* function, const char* file, int line,
                              const char* name, const glm::vec3& rgb, const glm::vec3& previous) -> bool {
        if (!IsFiniteVec3(rgb)) {
            return fail(stage, function, file, line, name, Vec3ToString(previous), Vec3ToString(rgb), "finite HDR check");
        }
        if (rgb.x < -1e-3f || rgb.y < -1e-3f || rgb.z < -1e-3f) {
            return fail(stage, function, file, line, name, Vec3ToString(previous), Vec3ToString(rgb), "non-negative radiance");
        }
        if (rgb.x > kMaxExpectedHdr || rgb.y > kMaxExpectedHdr || rgb.z > kMaxExpectedHdr) {
            return fail(stage, function, file, line, name, Vec3ToString(previous), Vec3ToString(rgb), "HDR magnitude check");
        }
        return true;
    };

    if (!checkRgb("transmittanceRGB", "IntegrateInscatteringDetailed", "AtmosphereIntegrator.hlsli", 135,
            "transmittanceRGB", sample.transmittanceRGB, glm::vec3(1.0f))) {
        return false;
    }
    if (!checkRgb("skyViewRGB", "WE_SampleSkyViewLUT", "AtmosphereLUT.hlsli", 47,
            "skyViewRGB", sample.skyViewRGB, glm::vec3(0.0f))) {
        return false;
    }
    if (!checkRgb("rayleighRGB", "WE_IntegrateInscatteringDetailed", "AtmosphereIntegrator.hlsli", 143,
            "rayleighRGB", sample.rayleighRGB, glm::vec3(0.0f))) {
        return false;
    }
    if (!checkRgb("mieRGB", "WE_IntegrateInscatteringDetailed", "AtmosphereIntegrator.hlsli", 144,
            "mieRGB", sample.mieRGB, glm::vec3(0.0f))) {
        return false;
    }
    if (!checkRgb("multiScatterRGB", "WE_IntegrateInscatteringDetailed", "AtmosphereIntegrator.hlsli", 145,
            "multiScatterRGB", sample.multiScatterRGB, glm::vec3(0.0f))) {
        return false;
    }
    if (!checkRgb("sunDiskRGB", "WE_ComputeSunDisk", "AtmosphereIntegrator.hlsli", 162,
            "sunDiskRGB", sample.sunDiskRGB, glm::vec3(0.0f))) {
        return false;
    }
    if (!checkRgb("finalHdrRGB", "WE_SampleSkyAtmosphereLUT", "AtmosphereLUT.hlsli", 98,
            "finalHdrRGB", sample.finalHdrRGB, sample.skyViewRGB)) {
        return false;
    }

    return true;
}

std::string AtmosphereRadianceTracer::FormatSampleReport(const AtmosphereRadianceStageSample& s) const {
    std::ostringstream ss;
    ss << "[" << s.label << " uv=" << Vec2ToString(s.screenUV) << "]\n"
       << "  ViewDirection=" << Vec3ToString(s.viewDirection) << " len=" << s.viewDirectionLength << "\n"
       << "  SunDirection=" << Vec3ToString(s.sunDirection) << " len=" << s.sunDirectionLength << "\n"
       << "  dot(View,Sun)=" << s.viewSunDot << "\n"
       << "  AtmosphereHeightKm=" << s.atmosphereHeightKm << "\n"
       << "  TransmittanceUV=" << Vec2ToString(s.transmittanceUV) << " RGB=" << Vec3ToString(s.transmittanceRGB) << "\n"
       << "  SkyViewUV=" << Vec2ToString(s.skyViewUV) << " RGB=" << Vec3ToString(s.skyViewRGB) << "\n"
       << "  Rayleigh=" << Vec3ToString(s.rayleighRGB) << "\n"
       << "  Mie=" << Vec3ToString(s.mieRGB) << "\n"
       << "  MultiScatter=" << Vec3ToString(s.multiScatterRGB) << "\n"
       << "  SunDiskMask=" << s.sunDiskMask << " SunRGB=" << Vec3ToString(s.sunDiskRGB) << "\n"
       << "  FinalHDR=" << Vec3ToString(s.finalHdrRGB);
    return ss.str();
}

std::string AtmosphereRadianceTracer::FormatFailureReport(const AtmosphereRadianceValidationFailure& f) const {
    std::ostringstream ss;
    ss << "FIRST INVALID STAGE: " << f.stage << "\n"
       << "  Function: " << f.function << "\n"
       << "  Source: " << f.sourceFile << ":" << f.sourceLine << "\n"
       << "  Variable: " << f.variable << "\n"
       << "  Previous valid: " << f.previousValid << "\n"
       << "  Invalid value: " << f.invalidValue << "\n"
       << "  Operation: " << f.operation;
    return ss.str();
}

void AtmosphereRadianceTracer::LogStandardTrace(
    const glm::mat4& view,
    const glm::mat4& proj,
    const glm::vec3& cameraPos,
    const glm::vec3& cameraForward,
    const glm::vec3& cameraRight,
    const glm::vec3& cameraUp,
    const SceneEnvironmentUniform& env) const {

    const auto samples = TraceStandardSamples(view, proj, cameraPos, cameraForward, cameraRight, cameraUp, env);
    for (const auto& sample : samples) {
        AtmosphereRadianceValidationFailure failure{};
        const bool ok = ValidateSample(sample, failure);
        WE_LOG_INFO(we::runtime::core::LogCategory::Environment.data(), FormatSampleReport(sample));
        if (!ok) {
            WE_LOG_ERROR(we::runtime::core::LogCategory::Environment.data(), FormatFailureReport(failure));
            break;
        }
    }
}

#endif // WE_HAS_GLM

} // namespace we::runtime::renderer
