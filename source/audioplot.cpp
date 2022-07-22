#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "implot.h"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include "audioplot_dr_mp3.h"
#include "audioplot_dr_wav.h"
#include "audioplot_pfd.h"
#include "audioplot_stb_vorbis.h"

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#define ARRSIZE(x) (sizeof(x) / (sizeof(x[0])))

// settings
const int kWindowWidth = 2400;
const int kWindowHeight = 1200;

const uint32_t kMaxDetailLevels = 16;
const uint64_t kMinDetailLevelPoints = 32768;

const ImPlotColormap kDefaultColorMap = ImPlotColormap_Dark;

typedef ImPlotPoint Point;
typedef ImVec4 Color;

class AudioData
{
public:
    AudioData(const char* filename)
    {
        loadFromFile(filename);
    }

    int32_t getNumChannels() const
    {
        return m_channelData.size();
    }

    uint64_t getNumValues() const
    {
        if (m_channelData.size() > 0) {
            return m_channelData[0].size();
        }
        else {
            return 0;
        }
    }

    double getValue(int32_t channel, uint64_t index) const
    {
        bool bValidChannel = (0 <= channel && channel < (int32_t)m_channelData.size());
        if (bValidChannel && index < m_channelData[channel].size()) {
            return m_channelData[channel][index];
        }
        return 0;
    }

    double getTime(uint64_t index) const
    {
        return index * m_samplePeriod;
    }

    double getMaxTime() const
    {
        return m_maxTime;
    }

    uint64_t getIndexForTime(double time) const
    {
        return (uint64_t)((time / m_samplePeriod) + 0.5);
    }

    int32_t numTraces() const
    {
        return (int32_t)m_traces.size();
    }

    const char* getTraceName(int32_t trace) const
    {
        if (0 <= trace && trace < (int32_t)m_channelNames.size()) {
            return m_channelNames[trace].c_str();
        }
        else {
            return "";
        }
    }

    bool isTraceVisible(int32_t trace) const
    {
        return m_bTraceVisibleBitmap & (1 << trace);
    }

    uint64_t getTracesVisibleBitmap() const
    {
        return m_bTraceVisibleBitmap;
    }

    void setTracesVisibleBitmap(uint64_t bTraceVisibleBitmap)
    {
        m_bTraceVisibleBitmap = bTraceVisibleBitmap;
    }

    void setAllTracesVisible(bool bVisible)
    {
        m_bTraceVisibleBitmap = (bVisible ? (uint64_t)(-1) : 0);
    }

    void toggleTraceVisible(int32_t trace)
    {
        m_bTraceVisibleBitmap = (m_bTraceVisibleBitmap ^ (1 << trace));
    }

    int32_t getNumVisibleTraces() const
    {
        int32_t numTraces = 0;
        for (int32_t i = 0; i < (int32_t)m_traces.size(); i++) {
            if (m_bTraceVisibleBitmap & (1 << i)) {
                numTraces++;
            }
        }
        return numTraces;
    }

    Color getTraceColor(int32_t trace) const
    {
        return ImPlot::GetColormapColor(trace);
    }

    uint64_t getNumPointsInRange(double range, int32_t level) const
    {
        if (m_traces.size() > 0) {
            double unscaledPointsForRange = getIndexForTime(range);
            unscaledPointsForRange = std::min((double)getNumPoints(0), unscaledPointsForRange);
            if (level == 0) {
                return (uint64_t)unscaledPointsForRange;
            }
            else {
                return (uint64_t)(unscaledPointsForRange / ((double)m_traces[0].m_levels[level].m_windowSize / 2.0));
            }
        }
        else {
            return 0;
        }
    }

    uint64_t getNumPoints(int32_t level) const
    {
        if (m_traces.size() > 0) {
            return m_traces[0].m_levels[level].m_points.size();
        }
        else {
            return 0;
        }
    }

    const Point* getPointArray(int32_t trace, int32_t level) const
    {
        return &m_traces[trace].m_levels[level].m_points[0];
    }

    uint32_t getNumLevels() const
    {
        return (uint32_t)m_traces[0].m_levels.size();
    }

private:
    struct TraceDetailLevel
    {
        std::vector<Point> m_points;
        uint64_t m_windowSize;
        double m_windowTime;
    };

    struct Trace
    {
        std::vector<TraceDetailLevel> m_levels;
    };

    uint64_t m_bTraceVisibleBitmap = 0;

    std::vector<std::string> m_channelNames;
    std::vector<std::vector<double>> m_channelData;
    std::vector<Trace> m_traces;

    double m_samplePeriod = 0.0;
    double m_maxTime = 0.0;

    void loadFromFile(const char* filename)
    {
        // std::cout << "Loading " << filename << "...\n";
        if (strstr(filename, ".wav") != NULL) {
            loadFromWavFile(filename);
        }
        else if (strstr(filename, ".mp3") != NULL) {
            loadFromMp3File(filename);
        }
        else if (strstr(filename, ".ogg") != NULL) {
            loadFromOggFile(filename);
        }
        // std::cout << "Finished loading.\n";
    }

    void loadFromMp3File(const char* filename)
    {
        unsigned int channelCount;
        unsigned int sampleRate;
        uint64_t frameCount;  // frame = 1 sample per channel
        float* pSampleData = openMp3FileAndReadPcmFramesF32(filename, &channelCount,
                                                            &sampleRate, &frameCount);
        if (pSampleData) {
            // std::cout << "    Loading .mp3 file with "
            //           << channelCount << " channels, "
            //           << frameCount << " frames at sample rate "
            //           << sampleRate << '\n';
            processF32Samples(pSampleData, channelCount, sampleRate, frameCount);
            freeMp3SampleData(pSampleData);
        }
    }

    void loadFromWavFile(const char* filename)
    {
        unsigned int channelCount;
        unsigned int sampleRate;
        uint64_t frameCount;  // frame = 1 sample per channel
        float* pSampleData = openWavFileAndReadPcmFramesF32(filename, &channelCount,
                                                            &sampleRate, &frameCount);
        if (pSampleData) {
            // std::cout << "    Loading .wav file with "
            //           << channelCount << " channels, "
            //           << frameCount << " frames at sample rate "
            //           << sampleRate << '\n';
            processF32Samples(pSampleData, channelCount, sampleRate, frameCount);
            freeWavSampleData(pSampleData);
        }
    }

    void loadFromOggFile(const char* filename)
    {
        unsigned int channelCount;
        unsigned int sampleRate;
        uint64_t frameCount;  // frame = 1 sample per channel
        float* pSampleData = openOggFileAndReadPcmFramesF32(filename, &channelCount,
                                                            &sampleRate, &frameCount);
        if (pSampleData) {
            // std::cout << "    Loading .ogg file with "
            //           << channelCount << " channels, "
            //           << frameCount << " frames at sample rate "
            //           << sampleRate << '\n';
            processF32Samples(pSampleData, channelCount, sampleRate, frameCount);
            freeOggSampleData(pSampleData);
        }
    }

    void processF32Samples(float* pSampleData, uint32_t channelCount, uint32_t sampleRate, uint64_t frameCount)
    {
        m_channelNames.reserve(channelCount);
        m_channelData.reserve(channelCount);

        for (size_t channel = 0; channel < channelCount; channel++) {
            std::string columnName = "Channel " + std::to_string(channel + 1);
            m_channelNames.push_back(columnName);

            std::vector<double> samples;
            samples.reserve(frameCount);

            for (size_t sample = 0; sample < frameCount; sample++) {
                const float value = pSampleData[(sample * channelCount) + channel];  // samples are interleaved
                samples.push_back((double)value);
            }

            m_channelData.push_back(std::move(samples));
        }

        if (sampleRate > 0) {
            m_samplePeriod = (1.0 / (double)sampleRate);
        }
        else {
            m_samplePeriod = 1.0;
        }

        m_maxTime = (double)((double)frameCount / (double)sampleRate);

        initializeTraceData();
    }

    TraceDetailLevel createDetailLevel(uint64_t windowSize, int32_t channel, uint64_t numValues) const
    {
        TraceDetailLevel level;
        level.m_windowSize = windowSize;
        level.m_windowTime = getTime(windowSize);

        // Resample using the min and max point in each window
        level.m_points.reserve(numValues + 1);
        for (uint64_t indexStart = 0; indexStart < numValues; indexStart += windowSize) {

            double xMin = 0.0;
            double xMax = 0.0;
            double yMin = DBL_MAX;
            double yMax = -DBL_MAX;
            const uint64_t indexEnd = std::min(indexStart + windowSize, numValues);
            for (uint64_t index = indexStart; index < indexEnd; index++) {
                const double y = getValue(channel, index); // -1 to +1
                if (y < yMin) {
                    xMin = getTime(index);
                    yMin = y;
                }
                if (y > yMax) {
                    xMax = getTime(index);
                    yMax = y;
                }
            }

            if (xMin < xMax) {
                level.m_points.push_back(Point(xMin, yMin));
                level.m_points.push_back(Point(xMax, yMax));
            }
            else {
                level.m_points.push_back(Point(xMax, yMax));
                level.m_points.push_back(Point(xMin, yMin));
            }
        }
        level.m_points.push_back(Point(getTime(numValues-1), getValue(channel, numValues-1)));

        return level;
    }

    void initializeTraceData()
    {
        // std::cout << "    Processing Channel Data...\n";

        const int32_t numChannels = getNumChannels();
        const uint64_t numValues = getNumValues();

        setAllTracesVisible(true);

        m_traces.reserve(numChannels);
        for (int32_t column = 0; column < numChannels; column++) {

            Trace trace;

            // Add full detail level
            {
                TraceDetailLevel level;
                level.m_windowSize = 1;
                level.m_windowTime = getMaxTime();
                level.m_points.reserve(numValues);
                for (uint64_t index = 0; index < numValues; index++) {
                    double x = getTime(index);
                    double y = getValue(column, index); // -1 to +1
                    level.m_points.push_back(Point(x, y));
                }
                trace.m_levels.push_back(std::move(level));
            }

            // Add summary detail levels
            uint64_t windowSize = 4;
            for (uint32_t i = 0; i < kMaxDetailLevels; i++) {
                if (trace.m_levels[i].m_points.size() < kMinDetailLevelPoints) {
                    break;
                }
                TraceDetailLevel level = createDetailLevel(windowSize, column, numValues);
                trace.m_levels.push_back(std::move(level));
                windowSize *= 2;
            }

            m_traces.push_back(std::move(trace));
            // std::cout << "        Channel " << column << " processed\n";
        }

        // std::cout << "    Finished Processing.\n";
    }
};

bool g_bMiddleMouseButtonPressed = false;
bool g_bCursorDecrLarge = false;
bool g_bCursorIncrLarge = false;
bool g_bCursorIncrSmall = false;
bool g_bCursorDecrSmall = false;
bool g_bXZoomInPressed = false;
bool g_bXZoomOutPressed = false;
bool g_bYZoomInPressed = false;
bool g_bYZoomOutPressed = false;
bool g_bYZoomResetPressed = false;
bool g_bYFitPressed = false;
bool g_bResetZoomPressed = false;
bool g_bPanLeftPressed = false;
bool g_bPanRightPressed = false;
bool g_bTraceShowAllPressed = false;
bool g_bTraceTogglePressed[20] = {};  // Keys 0-9, with and without shift
bool g_bTraceToggleExclusive = false;
bool g_bPlotModeSwitchPressed = false;
bool g_bColorMapPressed = false;

class GuiRenderer
{
public:
    GuiRenderer(AudioData& data, GLFWwindow* window)
    {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // Enable Multi-Viewport / Platform Windows
        io.ConfigFlags |= ImGuiViewportFlags_NoAutoMerge;

        // Setup Platform/Renderer bindings
#if defined(__APPLE__)
        const char* glsl_version = "#version 150";
#else
        const char* glsl_version = "#version 330 core";
#endif
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);

        // Setup Style
        ImGui::StyleColorsDark();
        ImPlot::PushColormap(m_colorMapIdx);

        m_frameCount = data.getNumValues();
        m_frameCurrent = m_frameCount / 2;

        m_plotMode = (data.numTraces() > 8 ? PLOT_MODE_COMBINED : PLOT_MODE_SPREAD);

        resetXAxis(data);
        resetYAxis();
    }

    void shutdown()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
    }

    void drawGui(AudioData& data)
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        //ImGui::GetIO().FontGlobalScale = 2.5;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

        processKeyboardCommands(data);

        drawColumnViewWindow(data);
        if (m_plotMode == PLOT_MODE_COMBINED || m_plotMode == PLOT_MODE_SPREAD) {
            drawCombinedPlotWindow(data);
        }
        else if (m_plotMode == PLOT_MODE_MULTIPLE) {
            drawMultiPlotWindow(data);
        }
        m_bPlotModeChanged = false;

        // ImGui::ShowMetricsWindow();

        // drawDebugWindow(data);

        ImGui::PopStyleVar();  // ImGuiStyleVar_WindowRounding

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
    }

    void processKeyboardCommands(AudioData& data)
    {
        // Handle Keyboard Combined/Multi Plot Toggle
        if (g_bPlotModeSwitchPressed) {
            g_bPlotModeSwitchPressed = false;
            m_plotMode = (PlotMode)((m_plotMode + 1) % NUM_PLOT_MODES);
            m_bPlotModeChanged = true;
        }

        // Handle Keyboard Trace Visibility Toggles
        if (g_bTraceShowAllPressed) {
            g_bTraceShowAllPressed = false;
            data.setAllTracesVisible(true);
        }
        for (int32_t i = 0; i < (int32_t)ARRSIZE(g_bTraceTogglePressed); i++) {
            if (g_bTraceTogglePressed[i]) {
                g_bTraceTogglePressed[i] = false;
                if (i < data.numTraces()) {
                    if (g_bTraceToggleExclusive) {
                        if (m_bExclusiveTraceMode) {
                            if (data.isTraceVisible(i)) {
                                data.setTracesVisibleBitmap(m_previousTracesVisibleBitmap);
                                m_bExclusiveTraceMode = false;
                            }
                            else {
                                data.setAllTracesVisible(false);
                                data.toggleTraceVisible(i);
                            }
                        }
                        else {
                            m_previousTracesVisibleBitmap = data.getTracesVisibleBitmap();
                            data.setAllTracesVisible(false);
                            data.toggleTraceVisible(i);
                            m_bExclusiveTraceMode = true;
                        }
                    }
                    else {
                        if (!m_bExclusiveTraceMode) {
                            data.toggleTraceVisible(i);
                        }
                    }
                }
            }
        }

        // Handle Keyboard Pan/Zoom Requests
        if (g_bResetZoomPressed) {
            g_bResetZoomPressed = false;
            resetXAxis(data);
            resetYAxis();
        }
        else if (g_bXZoomInPressed) {
            g_bXZoomInPressed = false;
            const double zoom = 0.2 * (m_xAxisMax - m_xAxisMin);
            m_xAxisMinNext += zoom;
            m_xAxisMaxNext -= zoom;
        }
        else if (g_bXZoomOutPressed) {
            g_bXZoomOutPressed = false;
            const double zoom = 0.2 * (m_xAxisMax - m_xAxisMin);
            m_xAxisMinNext -= zoom;
            m_xAxisMaxNext += zoom;
        }
        else if (g_bYZoomInPressed) {
            g_bYZoomInPressed = false;
            m_yAxisZoomLevel += 1;
            if (m_plotMode != PLOT_MODE_SPREAD) {
                m_yAxisMaxNext = yMaxForZoomLevel(m_yAxisZoomLevel);
                m_yAxisMinNext = -1.0 * m_yAxisMaxNext;
            }
        }
        else if (g_bYZoomOutPressed) {
            g_bYZoomOutPressed = false;
            m_yAxisZoomLevel -= 1;
            if (m_plotMode != PLOT_MODE_SPREAD) {
                m_yAxisMaxNext = yMaxForZoomLevel(m_yAxisZoomLevel);
                m_yAxisMinNext = -1.0 * m_yAxisMaxNext;
            }
        }
        else if (g_bYZoomResetPressed) {
            g_bYZoomResetPressed = false;
            resetYAxis();
        }
        else if (g_bPanRightPressed) {
            g_bPanRightPressed = false;
            const double pan = 0.2 * (m_xAxisMax - m_xAxisMin);
            m_xAxisMinNext += pan;
            m_xAxisMaxNext += pan;
        }
        else if (g_bPanLeftPressed) {
            g_bPanLeftPressed = false;
            const double pan = 0.2 * (m_xAxisMax - m_xAxisMin);
            m_xAxisMinNext -= pan;
            m_xAxisMaxNext -= pan;
        }
        else if (g_bYFitPressed) {
            g_bYFitPressed = false;
            m_bYFitRequested = true;
        }

        // Handle Keyboard Cursor Changes
        if (g_bCursorIncrLarge) {
            g_bCursorIncrLarge = false;
            const uint64_t frameIncr = (m_frameCount / 100);
            if (m_frameCurrent + frameIncr < m_frameCount + 1) {
                m_frameCurrent += frameIncr;
            }
            else {
                m_frameCurrent = m_frameCount - 1;
            }
        }
        else if (g_bCursorDecrLarge) {
            g_bCursorDecrLarge = false;
            const uint64_t frameIncr = (m_frameCount / 100);
            if (m_frameCurrent > frameIncr) {
                m_frameCurrent -= frameIncr;
            }
            else {
                m_frameCurrent = 0;
            }
        }
        else if (g_bCursorIncrSmall) {
            g_bCursorIncrSmall = false;
            if (m_frameCurrent < m_frameCount - 1) {
                m_frameCurrent += 1;
            }
        }
        else if (g_bCursorDecrSmall) {
            g_bCursorDecrSmall = false;
            if (m_frameCurrent > 1) {
                m_frameCurrent -= 1;
            }
            else {
                m_frameCurrent = 0;
            }
        }

        if (g_bColorMapPressed) {
            g_bColorMapPressed = false;
            cycleToNextColorMap();
        }
    }

    void drawColumnViewWindow(AudioData& data)
    {
        ImGuiViewport* pMainViewport = ImGui::GetMainViewport();
        ImVec2 size = ImVec2(pMainViewport->Size.x, pMainViewport->Size.y / 6.0);
        ImVec2 pos = ImVec2(pMainViewport->Pos.x, pMainViewport->Pos.y);
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::SetNextWindowViewport(pMainViewport->ID);
        ImGui::Begin("Column View Window", NULL,
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::PushItemWidth(-1);
        char lbl[64];
        snprintf(lbl, sizeof(lbl),
                 "Frame %" PRIu64 " / %" PRIu64 "          Time %.3f / %.3f",
                 m_frameCurrent + 1u, m_frameCount, data.getTime(m_frameCurrent), data.getMaxTime());
        static uint64_t min = 0;
        static uint64_t max = (m_frameCount - 1);
        ImGui::SliderScalar("", ImGuiDataType_U64, &m_frameCurrent, &min, &max, lbl);
        ImGui::PopItemWidth();

        ImGui::Columns(data.getNumChannels() + 2);

        uint64_t contextFrames = 3;
        uint64_t framesToDisplay = (2 * contextFrames) + 1;
        uint64_t minFrame = (m_frameCurrent < contextFrames ? 0 : m_frameCurrent - contextFrames);
        uint64_t maxFrame = (m_frameCurrent + contextFrames < m_frameCount - 1 ? m_frameCurrent + contextFrames : m_frameCount - 1);
        if ((maxFrame - minFrame) < framesToDisplay) {
            if (minFrame == 0) {
                maxFrame = framesToDisplay;
            }
            if (maxFrame == (m_frameCount - 1)) {
                minFrame = (m_frameCount - 1 - framesToDisplay);
            }
        }

        ImColor highlightColor = ImColor(255, 237, 255);
        ImColor defaultColor = ImColor(177, 177, 177);

        ImGui::Text("Frame");
        for (uint64_t frame = minFrame; frame <= maxFrame; frame++) {
            ImColor color = (frame == m_frameCurrent ? highlightColor : defaultColor);

            char txt[32];
            snprintf(txt, sizeof(txt), "%" PRIu64, frame);
            ImGui::TextColored(color, "%s", txt);
        }
        ImGui::NextColumn();

        ImGui::Text("Time");
        for (uint64_t frame = minFrame; frame <= maxFrame; frame++) {
            double timeFrame = data.getTime(frame);
            ImColor color = (frame == m_frameCurrent ? highlightColor : defaultColor);
            ImGui::TextColored(color, "%12.10f", timeFrame);
        }
        ImGui::NextColumn();

        for (int32_t trace = 0; trace < data.numTraces(); trace++) {
            const char* statusString = "";
            if (m_bExclusiveTraceMode && data.isTraceVisible(trace)) {
                statusString = " (E)";
            }
            else if (!m_bExclusiveTraceMode && !data.isTraceVisible(trace)) {
                statusString = " (H)";
            }
            ImGui::Text("%s%s", data.getTraceName(trace), statusString);
            for (uint64_t frame = minFrame; frame <= maxFrame; frame++) {
                ImColor traceColor = data.getTraceColor(trace);
                ImColor color = (frame == m_frameCurrent ? highlightColor : traceColor);
                ImGui::TextColored(color, "%12.8f", data.getValue(trace, frame));
            }
            ImGui::NextColumn();
        }

        ImGui::Columns(1);

        ImGui::End();
    }

    void fitYLimitsToData(const AudioData& data)
    {
        double yMax = -DBL_MAX;
        for (int32_t trace = 0; trace < data.numTraces(); trace++) {
            if (!data.isTraceVisible(trace)) {
                continue;
            }

            for (uint64_t ix = m_plotStartIdx; ix < m_plotEndIdx; ix++) {
                const double value = std::abs(data.getPointArray(trace, m_levelCurrent)[ix].y);
                if (value > yMax) {
                    yMax = value;
                }
            }
        }

        if (yMax != -DBL_MAX) {
            m_yAxisMinNext = -1.05 * yMax;
            m_yAxisMaxNext =  1.05 * yMax;
        }

        m_yAxisZoomLevel = zoomLevelForYMax(m_yAxisMaxNext);
    }

    void drawCombinedPlotWindow(AudioData& data)
    {
        ImGuiViewport* pMainViewport = ImGui::GetMainViewport();
        ImVec2 size = ImVec2(pMainViewport->Size.x, 5.0 * pMainViewport->Size.y / 6.0);
        ImVec2 pos = ImVec2(pMainViewport->Pos.x, pMainViewport->Pos.y + (pMainViewport->Size.y / 6.0));
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::SetNextWindowViewport(pMainViewport->ID);
        ImGui::Begin("Plot Window", NULL,
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoScrollWithMouse);

        const bool bSpreadEnabled = (m_plotMode == PLOT_MODE_SPREAD) && !m_bExclusiveTraceMode;
        const char* plotName = bSpreadEnabled ? "##SPREAD" : "##COMBINED";
        ImVec2 plotWindowSize = ImGui::GetContentRegionAvail();
        const ImPlotFlags plotFlags = ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect;

        if (ImPlot::BeginPlot(plotName, plotWindowSize, plotFlags)) {

            if (m_bYFitRequested) {
                m_bYFitRequested = false;
                fitYLimitsToData(data);
            }

            const bool bPlotLimitsChanged = processPlotLimitsChanges();

            if (bPlotLimitsChanged) {
                ImPlot::SetupAxisLimits(ImAxis_X1, m_xAxisMin, m_xAxisMax, ImGuiCond_Always);
                if (bSpreadEnabled) {
                    ImPlot::SetupAxisLimits(ImAxis_Y1, -1.0, 1.0, ImGuiCond_Always);
                }
                else {
                    ImPlot::SetupAxisLimits(ImAxis_Y1, m_yAxisMin, m_yAxisMax, ImGuiCond_Always);
                }
            }
            else {
                ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, data.getMaxTime(), ImGuiCond_Once);
                ImPlot::SetupAxisLimits(ImAxis_Y1, -1.0, 1.0, ImGuiCond_Once);
            }

            const ImPlotAxisFlags xAxisFlags = ImPlotAxisFlags_None;
            const ImPlotAxisFlags yAxisFlags = bSpreadEnabled ? (ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoTickLabels)
                                                              : (ImPlotAxisFlags_Lock);

            ImPlot::SetupAxes("Time (s)", NULL, xAxisFlags, yAxisFlags);
            ImPlot::SetupLegend(ImPlotLocation_North, ImPlotLegendFlags_Horizontal | ImPlotLegendFlags_Outside);

            const ImPlotRect plotLimits = ImPlot::GetPlotLimits();
            detectPlotLimitsChangesFromMouse(plotLimits);

            const double timeRange = plotLimits.X.Size();

            uint64_t numPointsVisible = data.getNumPointsInRange(timeRange, m_levelCurrent);

            if (bPlotLimitsChanged) {
                numPointsVisible = adjustPlotDetailLevel(data, timeRange, numPointsVisible);
                adjustDataBounds(data, plotLimits.X.Min, plotLimits.X.Max);
            }

            const bool bShowMarkers = numPointsVisible < 250;

            drawTraceLines(data, 0, data.numTraces(), bShowMarkers, bSpreadEnabled);

            updateCursorPosition(data);

            drawCursorLine(data);

            ImPlot::EndPlot();
        }

        ImGui::End();
    }

    void drawMultiPlotWindow(AudioData& data)
    {
        ImGuiViewport* pMainViewport = ImGui::GetMainViewport();
        ImVec2 size = ImVec2(pMainViewport->Size.x, 5.0 * pMainViewport->Size.y / 6.0);
        ImVec2 pos = ImVec2(pMainViewport->Pos.x, pMainViewport->Pos.y + (pMainViewport->Size.y / 6.0));
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::SetNextWindowViewport(pMainViewport->ID);
        ImGui::Begin("Plot Window", NULL,
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoScrollWithMouse);

        const int32_t numVisibleTraces = data.getNumVisibleTraces();
        const bool bPlotLimitsChanged = processPlotLimitsChanges();
        const ImPlotSubplotFlags subplotFlags = ImPlotSubplotFlags_NoResize |
                                                ImPlotSubplotFlags_ShareItems |
                                                ImPlotSubplotFlags_LinkCols |
                                                ImPlotSubplotFlags_LinkAllX;
        if (ImPlot::BeginSubplots("##Plots", numVisibleTraces, 1, ImGui::GetContentRegionAvail(), subplotFlags)) {
            for (int32_t trace = 0; trace < data.numTraces(); trace++) {
                if (!data.isTraceVisible(trace)) {
                    continue;
                }

                const ImPlotFlags plotFlags = ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect;
                if (ImPlot::BeginPlot("", ImVec2(), plotFlags)) {

                    if (trace == 0) {
                        ImPlot::SetupLegend(ImPlotLocation_North, ImPlotLegendFlags_Horizontal | ImPlotLegendFlags_Outside);
                    }

                    if (bPlotLimitsChanged) {
                        ImPlot::SetupAxisLimits(ImAxis_X1, m_xAxisMin, m_xAxisMax, ImGuiCond_Always);
                        ImPlot::SetupAxisLimits(ImAxis_Y1, m_yAxisMin, m_yAxisMax, ImGuiCond_Always);

                    }
                    else {
                        ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, data.getMaxTime(), ImGuiCond_Once);
                        ImPlot::SetupAxisLimits(ImAxis_Y1, -1.0, 1.0, ImGuiCond_Once);
                    }

                    const double yticks[] = {m_yAxisMin, 0.0, m_yAxisMax};
                    static char ylabelstrs[ARRSIZE(yticks)][32];
                    snprintf(ylabelstrs[0], sizeof(ylabelstrs[0]), "%.4lf", m_yAxisMin);
                    snprintf(ylabelstrs[1], sizeof(ylabelstrs[1]), "0.0");
                    snprintf(ylabelstrs[2], sizeof(ylabelstrs[2]), "%.4lf", m_yAxisMax);
                    const char* const ylabels[] = {ylabelstrs[0], ylabelstrs[1], ylabelstrs[2]};
                    ImPlot::SetupAxisTicks(ImAxis_Y1, yticks, ARRSIZE(yticks), ylabels);

                    const ImPlotAxisFlags xAxisFlags = ImPlotAxisFlags_None;
                    const ImPlotAxisFlags yAxisFlags = ImPlotAxisFlags_Lock;
                    ImPlot::SetupAxes("Time (s)", NULL, xAxisFlags, yAxisFlags);

                    const ImPlotRect plotLimits = ImPlot::GetPlotLimits();
                    detectPlotLimitsChangesFromMouse(plotLimits);

                    const double timeRange = plotLimits.X.Size();

                    uint64_t numPointsVisible = data.getNumPointsInRange(timeRange, m_levelCurrent);

                    const bool bFirstTrace = (trace == 0);
                    if (bPlotLimitsChanged && bFirstTrace) {
                        numPointsVisible = adjustPlotDetailLevel(data, timeRange, numPointsVisible);
                        adjustDataBounds(data, plotLimits.X.Min, plotLimits.X.Max);
                    }

                    const bool bShowMarkers = numPointsVisible < 250;
                    const bool bSpreadEnabled = false;

                    drawTraceLines(data, trace, trace + 1, bShowMarkers, bSpreadEnabled);

                    updateCursorPosition(data);

                    drawCursorLine(data);

                    ImPlot::EndPlot();
                }
            }
            ImPlot::EndSubplots();
        }
        ImGui::End();
    }

    struct SpreadLinePlot
    {
        SpreadLinePlot(const char* traceName, const Point* pointArray, uint64_t numPoints, double yScale, double yOffset)
        : m_traceName(traceName)
        , m_pointArray(pointArray)
        , m_numPoints(numPoints)
        , m_yScale(yScale)
        , m_yOffset(yOffset)
        {
        }

        void PlotLine() const
        {
            ImPlot::PlotLineG(m_traceName, &SpreadLinePlot::getPoint, (void*)this, m_numPoints);
        }

        static ImPlotPoint getPoint(void* data, int idx)
        {
            const SpreadLinePlot* _this = (SpreadLinePlot*)data;
            const Point* pointArray = _this->m_pointArray;
            Point p = pointArray[idx];
            p.y *= _this->m_yScale;
            p.y += _this->m_yOffset;
            return p;
        }

        const char* m_traceName;
        const Point* m_pointArray;
        const uint64_t m_numPoints;
        const double m_yScale;
        const double m_yOffset;
    };

    void drawTraceLines(AudioData& data, int32_t traceStart, int32_t traceEnd, bool bShowMarkers, bool bSpread)
    {
        if (bShowMarkers) {
            ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_Circle);
        }

        for (int32_t trace = traceStart; trace < traceEnd; trace++) {
            if (data.isTraceVisible(trace)) {
                ImPlot::PushStyleColor(ImPlotCol_Line, data.getTraceColor(trace));

                const Point* pointArray = data.getPointArray(trace, m_levelCurrent);
                const int numPoints = m_plotEndIdx - m_plotStartIdx;
                if (bSpread) {
                    const int32_t numTraces = traceEnd - traceStart;
                    const double yScale = (1.0 / (double)numTraces) * yMaxForZoomLevel(m_yAxisZoomLevel);
                    const double yOffset = (1.0 - ((trace + 0.5) * (2.0 / (double)numTraces)));
                    SpreadLinePlot slp(data.getTraceName(trace),
                                       &pointArray[m_plotStartIdx],
                                       numPoints, yScale, yOffset);
                    slp.PlotLine();
                }
                else {
                    const int offset = 0;
                    const size_t stride = sizeof(Point);
                    ImPlot::PlotLine(data.getTraceName(trace),
                                     &pointArray[m_plotStartIdx].x, &pointArray[m_plotStartIdx].y,
                                     numPoints, offset, stride);
                }

                ImPlot::PopStyleColor(1);
            }
        }

        if (bShowMarkers) {
            ImPlot::PopStyleVar(1);
        }
    }

    uint64_t adjustPlotDetailLevel(AudioData& data, double timeRange, uint64_t numPointsVisible)
    {
        // Try to decrease detail level (make fewer points visible)
        while (m_levelCurrent + 1 < data.getNumLevels()) {
            uint32_t numPointsVisibleNextLevel = data.getNumPointsInRange(timeRange, m_levelCurrent + 1);
            if (numPointsVisibleNextLevel > (kMinDetailLevelPoints / 2)) {
                m_levelCurrent = (m_levelCurrent + 1);
                numPointsVisible = numPointsVisibleNextLevel;
            }
            else {
                break;
            }
        }

        // Try to increase detail level (make more points visible)
        while (m_levelCurrent > 0) {
            uint32_t numPointsVisiblePrevLevel = data.getNumPointsInRange(timeRange, m_levelCurrent - 1);
            if (numPointsVisiblePrevLevel < (kMinDetailLevelPoints / 2)) {
                m_levelCurrent = (m_levelCurrent - 1);
                numPointsVisible = numPointsVisiblePrevLevel;
            }
            else {
                break;
            }
        }

        return numPointsVisible;
    }

    void adjustDataBounds(AudioData& data, double xMin, double xMax)
    {
        // Cull points to avoid segfault when there are > 2^32 points
        const Point* pointArray = data.getPointArray(0, m_levelCurrent);
        const uint64_t numPoints = data.getNumPoints(m_levelCurrent);
        const Point* pArrayStart = &pointArray[0];
        const Point* pArrayEnd = &pointArray[numPoints];

        const Point* pPlotStart = std::lower_bound(pArrayStart, pArrayEnd, xMin,
                                                   [](const Point& p, double limit) { return p.x < limit; });

        m_plotStartIdx = (uint64_t)(pPlotStart - pArrayStart);
        if (m_plotStartIdx > 0) {
            m_plotStartIdx--;
        }

        const Point* pPlotEnd = std::upper_bound(pPlotStart, pArrayEnd, xMax,
                                                 [](double limit, const Point& p) { return limit < p.x; });

        m_plotEndIdx = (uint64_t)(pPlotEnd - pArrayStart);
        if (m_plotEndIdx < numPoints) {
            m_plotEndIdx++;
        }
    }

    void updateCursorPosition(AudioData& data)
    {
        if (g_bMiddleMouseButtonPressed) {
            ImPlotPoint plotMousePos = ImPlot::GetPlotMousePos();
            if (plotMousePos.x <= 0) {
                m_frameCurrent = 0;
            }
            else if (plotMousePos.x >= data.getMaxTime()) {
                m_frameCurrent = m_frameCount - 1;
            }
            else {
                m_frameCurrent = data.getIndexForTime(ImPlot::GetPlotMousePos().x);
            }
        }
    }

    void drawCursorLine(AudioData& data)
    {
        const double timeCurrent = data.getTime(m_frameCurrent);
        const double cursorPosX[] = {timeCurrent};
        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(255, 255, 255, 255));
        ImPlot::PlotVLines("##Cursor", cursorPosX, 1);
        ImPlot::PopStyleColor();
    }

    bool processPlotLimitsChanges()
    {
        const bool bPlotLimitsChanged = (m_xAxisMin != m_xAxisMinNext) || (m_xAxisMax != m_xAxisMaxNext) ||
                                        (m_yAxisMin != m_yAxisMinNext) || (m_yAxisMax != m_yAxisMaxNext) ||
                                        m_bPlotModeChanged;
        m_xAxisMin = m_xAxisMinNext;
        m_xAxisMax = m_xAxisMaxNext;
        m_yAxisMax = std::max(std::abs(m_yAxisMaxNext), std::abs(m_yAxisMinNext));
        m_yAxisMin = -1.0 * m_yAxisMax;

        return bPlotLimitsChanged;
    }

    void detectPlotLimitsChangesFromMouse(const ImPlotRect& plotLimits)
    {
        if ((plotLimits.X.Min != m_xAxisMin) || (plotLimits.X.Max != m_xAxisMax)) {
            m_xAxisMinNext = plotLimits.X.Min;
            m_xAxisMaxNext = plotLimits.X.Max;
        }
        if ((plotLimits.Y.Min != m_yAxisMin) || (plotLimits.Y.Max != m_yAxisMax)) {
            m_yAxisMinNext = plotLimits.Y.Min;
            m_yAxisMaxNext = plotLimits.Y.Max;
        }
    }

    void resetXAxis(AudioData& data)
    {
        m_xAxisMinNext = 0;
        m_xAxisMaxNext = data.getMaxTime();
    }

    void resetYAxis()
    {
        m_yAxisMinNext = -1.0;
        m_yAxisMaxNext = 1.0;
        m_yAxisZoomLevel = 0;
    }

    void cycleToNextColorMap()
    {
        m_colorMapIdx = ((m_colorMapIdx + 1) % ImPlot::GetColormapCount());
        ImPlot::PopColormap();
        ImPlot::PushColormap(m_colorMapIdx);
    }

	void drawDebugWindow(AudioData& data)
    {
        ImGui::Begin("Debug Window", NULL);
        ImGui::Text("%20s : %f", "m_xAxisMin", m_xAxisMin);
        ImGui::Text("%20s : %f", "m_xAxisMin", m_xAxisMin);
        ImGui::Text("%20s : %f", "m_xAxisMax", m_xAxisMax);
        ImGui::Text("%20s : %f", "m_xAxisMinNext", m_xAxisMinNext);
        ImGui::Text("%20s : %f", "m_xAxisMaxNext", m_xAxisMaxNext);
        ImGui::Text("%20s : %f", "m_yAxisMin", m_yAxisMin);
        ImGui::Text("%20s : %f", "m_yAxisMax", m_yAxisMax);
        ImGui::Text("%20s : %f", "m_yAxisMinNext", m_yAxisMinNext);
        ImGui::Text("%20s : %f", "m_yAxisMaxNext", m_yAxisMaxNext);
        ImGui::Text("%20s : %" PRIi32, "m_yAxisZoomLevel", m_yAxisZoomLevel);
        ImGui::Text("%20s : %" PRIu64, "m_plotStartIdx", m_plotStartIdx);
        ImGui::Text("%20s : %" PRIu64, "m_plotEndIdx", m_plotEndIdx);
        ImGui::Text("%20s : %" PRIu32, "m_levelCurrent", m_levelCurrent);
        ImGui::Text("%20s : %" PRIu64, "m_frameCurrent", m_frameCurrent);
        ImGui::Text("%20s : %" PRIu64, "m_frameCount", m_frameCount);
        ImGui::Text("%20s : %" PRIi32, "data.getNumChannels()", data.getNumChannels());
        ImGui::Text("%20s : %" PRIu64, "data.getNumValues()", data.getNumValues());
        ImGui::Text("%20s : %f", "data.getMaxTime()", data.getMaxTime());
        ImGui::Text("%20s : %" PRIi32, "data.numTraces()", data.numTraces());
        ImGui::Text("%20s : 0x%" PRIx64, "data.getTracesVisibleBitmap()", data.getTracesVisibleBitmap());
        ImGui::Text("%20s : %" PRIi32, "data.getNumVisibleTraces()", data.getNumVisibleTraces());
        ImGui::Text("%20s : %" PRIu32, "data.getNumLevels()", data.getNumLevels());
        for (uint32_t level = 0; level < data.getNumLevels(); level++) {
            ImGui::Text("data.getNumPoints(%d) : %" PRIu64, level, data.getNumPoints(level));
        }

        ImGui::End();
    }
	
    static double yMaxForZoomLevel(int32_t level)
    {
        return std::pow(1.2, level);
    }

    static int32_t zoomLevelForYMax(double yMax)
    {
        int32_t level = 0;
        while (yMaxForZoomLevel(level) < yMax && yMaxForZoomLevel(level + 1) < yMax) {
            level += 1;
        }
        while (yMaxForZoomLevel(level) > yMax && yMaxForZoomLevel(level - 1) > yMax) {
            level -= 1;
        }
        return level;
    }


private:
    enum PlotMode
    {
        PLOT_MODE_COMBINED,
        PLOT_MODE_SPREAD,
        PLOT_MODE_MULTIPLE,
        NUM_PLOT_MODES,
    };

    PlotMode m_plotMode = PLOT_MODE_COMBINED;
    bool m_bPlotModeChanged = false;
    bool m_bExclusiveTraceMode = false;
    bool m_bYFitRequested = false;
    uint64_t m_previousTracesVisibleBitmap = 0;
    double m_xAxisMin = 0;
    double m_xAxisMax = 0;
    double m_xAxisMinNext = 0;
    double m_xAxisMaxNext = 0;
    double m_yAxisMin = 0;
    double m_yAxisMax = 0;
    double m_yAxisMinNext = 0;
    double m_yAxisMaxNext = 0;
    int32_t m_yAxisZoomLevel = 0;
    uint64_t m_plotStartIdx = 0;
    uint64_t m_plotEndIdx = 0;
    uint32_t m_levelCurrent = 0;
    uint64_t m_frameCurrent = 0;
    uint64_t m_frameCount = 0;
    ImPlotColormap m_colorMapIdx = kDefaultColorMap;
};

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    (void)window;
    (void)mods;
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) {
            g_bMiddleMouseButtonPressed = true;
        }
        else if (action == GLFW_RELEASE) {
            g_bMiddleMouseButtonPressed = false;
        }
    }
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)mods;
    if (action == GLFW_PRESS) {
        switch(key) {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, true);
                break;
            case GLFW_KEY_LEFT:
                g_bCursorDecrLarge = true;
                break;
            case GLFW_KEY_RIGHT:
                g_bCursorIncrLarge = true;
                break;
            case GLFW_KEY_UP:
                g_bCursorIncrSmall = true;
                break;
            case GLFW_KEY_DOWN:
                g_bCursorDecrSmall = true;
                break;
            case GLFW_KEY_SPACE:
                g_bResetZoomPressed = true;
                break;
            case GLFW_KEY_W:
                g_bXZoomInPressed = true;
                break;
            case GLFW_KEY_A:
                g_bPanLeftPressed = true;
                break;
            case GLFW_KEY_S:
                g_bXZoomOutPressed = true;
                break;
            case GLFW_KEY_D:
                g_bPanRightPressed = true;
                break;
            case GLFW_KEY_Q:
                g_bYZoomOutPressed = true;
                break;
            case GLFW_KEY_E:
                g_bYZoomInPressed = true;
                break;
            case GLFW_KEY_F:
                g_bYFitPressed = true;
                break;
            case GLFW_KEY_R:
                g_bYZoomResetPressed = true;
                break;
            case GLFW_KEY_C:
                g_bColorMapPressed = true;
                break;
            case GLFW_KEY_1:
            case GLFW_KEY_2:
            case GLFW_KEY_3:
            case GLFW_KEY_4:
            case GLFW_KEY_5:
            case GLFW_KEY_6:
            case GLFW_KEY_7:
            case GLFW_KEY_8:
            case GLFW_KEY_9:
                g_bTraceToggleExclusive = !(mods & GLFW_MOD_CONTROL);
                g_bTraceTogglePressed[key - GLFW_KEY_1 + (mods & GLFW_MOD_SHIFT ? 10 : 0)] = true;
                break;
            case GLFW_KEY_0:
                g_bTraceToggleExclusive = !(mods & GLFW_MOD_CONTROL);
                g_bTraceTogglePressed[9 + (mods & GLFW_MOD_SHIFT ? 10 : 0)] = true;
                break;
                break;
            case GLFW_KEY_GRAVE_ACCENT:
                g_bTraceShowAllPressed = true;
                break;
            case GLFW_KEY_TAB:
                g_bPlotModeSwitchPressed = true;
                break;
        }
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    (void)window;
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void errorCallback(int error, const char* description)
{
    std::cerr << "Error " << error << " : " << description << std::endl;
}

int main(int argc, const char** argv)
{
    std::string filename;
    if (argc > 2) {
        return -1;
    }
    else if (argc == 2) {
        // Load the filename provided
        filename = argv[1];
    }
    else {
        filename = promptForFilename();
    }

    if (filename == "") {
        std::cerr << "No file selected\n";
        return -1;
    }

    // Load the data to plotted
    AudioData audioData(filename.c_str());

    if (audioData.getNumValues() == 0) {
        std::cerr << "Unable to load file: " << filename << "\n";
        return -1;
    }

    // std::cout << "Initializing GUI...\n");

    // glfw: initialize and configure
    // ------------------------------
    glfwSetErrorCallback(errorCallback);
    glfwInit();
#if defined(__APPLE__)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

    // glfw window creation
    // --------------------
    char windowTitle[512];
    snprintf(windowTitle, sizeof(windowTitle), "%s - Audio Plot", filename.c_str());
    GLFWwindow* window = glfwCreateWindow(kWindowWidth, kWindowHeight, windowTitle, NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetKeyCallback(window, keyCallback);

    // glw3: load all OpenGL function pointers
    // ---------------------------------------
    if (gl3wInit() != 0)
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    GuiRenderer guiRenderer(audioData, window);

    // std::cout << "Finished Initializing.\n");

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(1.0, 1.0, 1.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        guiRenderer.drawGui(audioData);

        glfwSwapBuffers(window);
        glfwWaitEvents();
    }

    // clean up
    // --------
    guiRenderer.shutdown();

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
