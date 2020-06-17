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
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

// settings
const int kWindowWidth = 2400;
const int kWindowHeight = 1200;

const uint32_t kMaxDetailLevels = 16;
const uint64_t kMinDetailLevelPoints = 32768;

const int32_t kMaxChannels = 16;

typedef ImPlotPoint Point;
typedef ImVec4 Color;

class AudioData
{
public:
    AudioData(const char* filename)
    {
        loadFromFile(filename);
    }

    int32_t numColumns() const
    {
        return m_columnData.size();
    }

    uint64_t numValues() const
    {
        if (m_columnData.size() > 0) {
            return m_columnData[0].size();
        }
        else {
            return 0;
        }
    }

    double getValue(int32_t column, uint64_t index) const
    {
        bool bValidColumn = (0 <= column && column < (int32_t)m_columnData.size());
        if (bValidColumn && index < m_columnData[column].size()) {
            return m_columnData[column][index];
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
        if (0 <= trace && trace < (int32_t)m_columnNames.size()) {
            return m_columnNames[trace].c_str();
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

    int32_t numVisibleTraces() const
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
        return m_traces[trace].m_color;
    }

    uint64_t numPointsInRange(double range, int32_t level) const
    {
        if (m_traces.size() > 0) {
            double unscaledPointsForRange = getIndexForTime(range);
            unscaledPointsForRange = MIN(numPoints(0), unscaledPointsForRange);
            if (level == 0) {
                return unscaledPointsForRange;
            }
            else {
                return (uint64_t)(unscaledPointsForRange / ((double)m_traces[0].m_levels[level].m_windowSize / 2.0));
            }
        }
        else {
            return 0;
        }
    }

    uint64_t numPoints(int32_t level) const
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

    const Point* getSpreadPointArray(int32_t trace, int32_t level) const
    {
        return &m_traces[trace].m_levels[level].m_spreadPoints[0];
    }

    uint32_t numLevels() const
    {
        return (uint32_t)m_traces[0].m_levels.size();
    }

private:
    struct TraceDetailLevel
    {
        std::vector<Point> m_points;
        std::vector<Point> m_spreadPoints;
        uint64_t m_windowSize;
        double m_windowTime;
    };

    struct Trace
    {
        std::vector<TraceDetailLevel> m_levels;
        Color m_color;
    };

    uint64_t m_bTraceVisibleBitmap = 0;

    std::vector<std::string> m_columnNames;
    std::vector<std::vector<double>> m_columnData;
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
        m_columnNames.reserve(channelCount);
        m_columnData.reserve(channelCount);

        for (size_t channel = 0; channel < channelCount; channel++) {
            std::string columnName = "Channel " + std::to_string(channel + 1);
            m_columnNames.push_back(columnName);

            std::vector<double> samples;
            samples.reserve(frameCount);

            for (size_t sample = 0; sample < frameCount; sample++) {
                const float value = pSampleData[(sample * channelCount) + channel];  // samples are interleaved
                samples.push_back((double)value);
            }

            m_columnData.push_back(std::move(samples));
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

    static Color getDefaultColor(int32_t trace)
    {
        static const Color colors[] = {
            // https://colorbrewer2.org/#type=qualitative&scheme=Set1&n=8
            { (228.0 / 255.0), ( 26.0 / 255.0), ( 28.0 / 255.0), 1.0 },  // red
            { ( 55.0 / 255.0), (126.0 / 255.0), (184.0 / 255.0), 1.0 },  // blue
            { ( 77.0 / 255.0), (175.0 / 255.0), ( 74.0 / 255.0), 1.0 },  // green
            { (152.0 / 255.0), ( 78.0 / 255.0), (163.0 / 255.0), 1.0 },  // purple
            { (255.0 / 255.0), (127.0 / 255.0), (  0.0 / 255.0), 1.0 },  // orange
            { (153.0 / 255.0), (153.0 / 255.0), (153.0 / 255.0), 1.0 },  // gray
            { (166.0 / 255.0), ( 86.0 / 255.0), ( 40.0 / 255.0), 1.0 },  // brown
            { (247.0 / 255.0), (129.0 / 255.0), (191.0 / 255.0), 1.0 },  // pink
            // https://colorbrewer2.org/#type=qualitative&scheme=Pastel1&n=8
            { (251.0 / 255.0), (180.0 / 255.0), (174.0 / 255.0), 1.0 },
            { (179.0 / 255.0), (205.0 / 255.0), (227.0 / 255.0), 1.0 },
            { (204.0 / 255.0), (235.0 / 255.0), (197.0 / 255.0), 1.0 },
            { (222.0 / 255.0), (203.0 / 255.0), (228.0 / 255.0), 1.0 },
            { (254.0 / 255.0), (217.0 / 255.0), (166.0 / 255.0), 1.0 },
            { (255.0 / 255.0), (255.0 / 255.0), (204.0 / 255.0), 1.0 },
            { (229.0 / 255.0), (216.0 / 255.0), (189.0 / 255.0), 1.0 },
            { (253.0 / 255.0), (218.0 / 255.0), (236.0 / 255.0), 1.0 },
        };
        return colors[trace % ARRSIZE(colors)];
    }

    TraceDetailLevel createDetailLevel(uint64_t windowSize, int32_t column, uint64_t numTimeSeriesValues) const
    {
        TraceDetailLevel level;
        level.m_windowSize = windowSize;
        level.m_windowTime = getTime(windowSize);

        // Resample using the min and max point in each window
        level.m_points.reserve(numTimeSeriesValues);
        for (uint64_t indexStart = 0; indexStart < (numTimeSeriesValues + windowSize); indexStart += windowSize) {

            double xMin = 0.0;
            double xMax = 0.0;
            double yMin = DBL_MAX;
            double yMax = -DBL_MAX;
            const uint64_t indexEnd = MIN(indexStart + windowSize, numTimeSeriesValues);
            for (uint64_t index = indexStart; index < indexEnd; index++) {
                const double y = getValue(column, index); // -1 to +1
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
                level.m_points.push_back(Point({xMin, yMin}));
                level.m_points.push_back(Point({xMax, yMax}));
            }
            else {
                level.m_points.push_back(Point({xMax, yMax}));
                level.m_points.push_back(Point({xMin, yMin}));
            }
        }

        return level;
    }

    void initializeTraceData()
    {
        // std::cout << "    Processing Channel Data...\n";

        const int32_t numTimeSeriesColumns = numColumns();
        const uint64_t numTimeSeriesValues = numValues();

        setAllTracesVisible(true);

        m_traces.reserve(numTimeSeriesColumns);
        for (int32_t column = 0; column < numTimeSeriesColumns; column++) {

            Trace trace;
            trace.m_color = getDefaultColor(column);

            // Add full detail level
            {
                TraceDetailLevel level;
                level.m_windowSize = 1;
                level.m_windowTime = getMaxTime();
                level.m_points.reserve(numTimeSeriesValues);
                for (uint64_t index = 0; index < numTimeSeriesValues; index++) {
                    double x = getTime(index);
                    double y = getValue(column, index); // -1 to +1
                    level.m_points.push_back(Point({x, y}));
                }
                trace.m_levels.push_back(std::move(level));
            }

            // Add summary detail levels
            uint64_t windowSize = 4;
            for (uint32_t i = 0; i < kMaxDetailLevels; i++) {
                if (trace.m_levels[i].m_points.size() < kMinDetailLevelPoints) {
                    break;
                }
                TraceDetailLevel level = createDetailLevel(windowSize, column, numTimeSeriesValues);
                trace.m_levels.push_back(std::move(level));
                windowSize *= 2;
            }

            m_traces.push_back(std::move(trace));
            // std::cout << "        Channel " << column << " processed\n";
        }

        const int32_t numTraces = m_traces.size();
        const double scale = (1.0 / (double)numTraces);
        for (int32_t trace = 0; trace < numTraces; trace++) {

            const double offset = (1.0 - ((trace + 0.5) * (2.0 / (double)numTraces)));
            for (uint32_t level = 0; level < m_traces[trace].m_levels.size(); level++) {

                const size_t numPoints = m_traces[trace].m_levels[level].m_points.size();
                m_traces[trace].m_levels[level].m_spreadPoints.reserve(numPoints);

                for (uint64_t point = 0; point < numPoints; point++) {

                    Point p = m_traces[trace].m_levels[level].m_points[point];
                    p.y *= scale;
                    p.y += offset;
                    m_traces[trace].m_levels[level].m_spreadPoints.push_back(p);
                }
            }
        }

        // std::cout << "    Finished Processing.\n";
    }
};

bool g_bMiddleMouseButtonPressed = false;
bool g_bCursorDecrLarge = false;
bool g_bCursorIncrLarge = false;
bool g_bCursorIncrSmall = false;
bool g_bCursorDecrSmall = false;
bool g_bZoomInPressed = false;
bool g_bZoomOutPressed = false;
bool g_bResetZoomPressed = false;
bool g_bPanLeftPressed = false;
bool g_bPanRightPressed = false;
bool g_bTraceShowAllPressed = false;
bool g_bTraceTogglePressed[20] = {};  // Keys 0-9, with and without shift
bool g_bTraceToggleExclusive = false;
bool g_bPlotModeSwitchPressed = false;

class GuiRenderer
{
public:
    GuiRenderer(AudioData& data, GLFWwindow* window)
    {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // Enable Multi-Viewport / Platform Windows
        io.ConfigFlags |= ImGuiViewportFlags_NoAutoMerge;

        // Setup Platform/Renderer bindings
        const char* glsl_version = "#version 330 core";
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);

        // Setup Style
        ImGui::StyleColorsDark();

        m_frameCount = data.numValues();
        m_frameCurrent = m_frameCount / 2;

        m_plotMode = (data.numTraces() > 8 ? PLOT_MODE_COMBINED : PLOT_MODE_SPREAD);
    }

    void shutdown()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
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
            m_xAxisMinNext = 0;
            m_xAxisMaxNext = data.getMaxTime();
        }
        else if (g_bZoomInPressed) {
            g_bZoomInPressed = false;
            const double zoom = 0.2 * (m_xAxisMax - m_xAxisMin);
            m_xAxisMinNext += zoom;
            m_xAxisMaxNext -= zoom;
        }
        else if (g_bZoomOutPressed) {
            g_bZoomOutPressed = false;
            const double zoom = 0.2 * (m_xAxisMax - m_xAxisMin);
            m_xAxisMinNext -= zoom;
            m_xAxisMaxNext += zoom;
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

        ImGui::Columns(data.numColumns() + 2);

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

        const bool bPlotLimitsChanged = (m_xAxisMin != m_xAxisMinNext) || (m_xAxisMax != m_xAxisMaxNext) || m_bPlotModeChanged;
        m_xAxisMin = m_xAxisMinNext;
        m_xAxisMax = m_xAxisMaxNext;

        if (bPlotLimitsChanged) {
            ImPlot::SetNextPlotLimitsX(m_xAxisMin, m_xAxisMax, ImGuiCond_Always);
        }
        else {
            ImPlot::SetNextPlotLimitsX(0.0, data.getMaxTime(), ImGuiCond_Once);
        }
        ImPlot::SetNextPlotLimitsY(-1.0, 1.0, ImGuiCond_Always);

        ImVec2 plotWindowSize = ImGui::GetContentRegionAvail();

        const bool bSpreadEnabled = (m_plotMode == PLOT_MODE_SPREAD) && !m_bExclusiveTraceMode;
        const ImPlotFlags plotFlags = ImPlotFlags_Default;
        const ImPlotAxisFlags xAxisFlags = ImPlotAxisFlags_Default;
        const ImPlotAxisFlags yAxisFlags = bSpreadEnabled ? (ImPlotAxisFlags_Default & ~ImPlotAxisFlags_TickLabels)
                                                          : (ImPlotAxisFlags_Default);
        const char* plotName = bSpreadEnabled ? "##SPREAD" : "##COMBINED";

        if (ImPlot::BeginPlot(plotName, "Time (s)", NULL, plotWindowSize, plotFlags, xAxisFlags, yAxisFlags)) {

            const ImPlotLimits plotLimits = ImPlot::GetPlotLimits();
            if ((plotLimits.X.Min != m_xAxisMin) || (plotLimits.X.Max != m_xAxisMax)) {
                m_xAxisMinNext = plotLimits.X.Min;
                m_xAxisMaxNext = plotLimits.X.Max;
            }

            const double timeRange = plotLimits.X.Size();

            uint64_t numPointsVisible = data.numPointsInRange(timeRange, m_levelCurrent);

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

        const int32_t numVisibleTraces = data.numVisibleTraces();
        const ImVec2 childSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y / numVisibleTraces);

        const bool bPlotLimitsChanged = (m_xAxisMin != m_xAxisMinNext) || (m_xAxisMax != m_xAxisMaxNext) || m_bPlotModeChanged;
        m_xAxisMin = m_xAxisMinNext;
        m_xAxisMax = m_xAxisMaxNext;

        int32_t traceCount = 0;
        for (int32_t trace = 0; trace < data.numTraces(); trace++) {
            if (!data.isTraceVisible(trace)) {
                continue;
            }
            traceCount++;

            char childName[32];
            snprintf(childName, sizeof(childName), "Child%" PRIi32, trace);
            if (ImGui::BeginChild(childName, childSize)) {

                if (bPlotLimitsChanged) {
                    ImPlot::SetNextPlotLimitsX(m_xAxisMin, m_xAxisMax, ImGuiCond_Always);
                }
                else {
                    ImPlot::SetNextPlotLimitsX(0.0, data.getMaxTime(), ImGuiCond_Once);
                }
                ImPlot::SetNextPlotLimitsY(-1.0, 1.0, ImGuiCond_Always);

                char plotName[32];
                snprintf(plotName, sizeof(plotName), "##Plot%" PRIi32, trace);
                const char* xAxisLabel = (traceCount == numVisibleTraces ? "Time (s)" : NULL);
                if (ImPlot::BeginPlot(plotName, xAxisLabel, NULL, childSize)) {

                    const ImPlotLimits plotLimits = ImPlot::GetPlotLimits();
                    if ((plotLimits.X.Min != m_xAxisMin) || (plotLimits.X.Max != m_xAxisMax)) {
                        m_xAxisMinNext = plotLimits.X.Min;
                        m_xAxisMaxNext = plotLimits.X.Max;
                    }

                    const double timeRange = plotLimits.X.Size();

                    uint64_t numPointsVisible = data.numPointsInRange(timeRange, m_levelCurrent);

                    if (bPlotLimitsChanged && (traceCount == 1)) {
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
                ImGui::EndChild();
            }
        }
        ImGui::End();
    }

    void drawTraceLines(AudioData& data, int32_t traceStart, int32_t traceEnd, bool bShowMarkers, bool bSpread)
    {
        if (bShowMarkers) {
            ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_Circle);
        }

        for (int32_t trace = traceStart; trace < traceEnd; trace++) {
            if (data.isTraceVisible(trace)) {
                ImPlot::PushStyleColor(ImPlotCol_Line, data.getTraceColor(trace));
                const Point* pointArray = bSpread ? data.getSpreadPointArray(trace, m_levelCurrent)
                                                  : data.getPointArray(trace, m_levelCurrent);
                ImPlot::PlotLine(data.getTraceName(trace), &pointArray[m_plotStartIdx], m_plotEndIdx - m_plotStartIdx);
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
        while (m_levelCurrent + 1 < data.numLevels()) {
            uint32_t numPointsVisibleNextLevel = data.numPointsInRange(timeRange, m_levelCurrent + 1);
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
            uint32_t numPointsVisiblePrevLevel = data.numPointsInRange(timeRange, m_levelCurrent - 1);
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
        const uint64_t numPoints = data.numPoints(m_levelCurrent);
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
        ImVec2 rmin = ImPlot::PlotToPixels(ImPlotPoint(timeCurrent, -1.0));
        ImVec2 rmax = ImPlot::PlotToPixels(ImPlotPoint(timeCurrent,  1.0));
        ImPlot::PushPlotClipRect();
        ImGui::GetWindowDrawList()->AddLine(rmin, rmax, ImColor(255, 255, 255));
        ImPlot::PopPlotClipRect();
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
    uint64_t m_previousTracesVisibleBitmap = 0;
    double m_xAxisMin = 0;
    double m_xAxisMax = 0;
    double m_xAxisMinNext = 0;
    double m_xAxisMaxNext = 0;
    uint64_t m_plotStartIdx = 0;
    uint64_t m_plotEndIdx = 0;
    uint32_t m_levelCurrent = 0;
    uint64_t m_frameCurrent = 0;
    uint64_t m_frameCount = 0;
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
                g_bZoomInPressed = true;
                break;
            case GLFW_KEY_A:
                g_bPanLeftPressed = true;
                break;
            case GLFW_KEY_S:
                g_bZoomOutPressed = true;
                break;
            case GLFW_KEY_D:
                g_bPanRightPressed = true;
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

    if (audioData.numValues() == 0) {
        std::cerr << "Unable to load file: " << filename << "\n";
        return -1;
    }

    // std::cout << "Initializing GUI...\n");

    // glfw: initialize and configure
    // ------------------------------
    glfwSetErrorCallback(errorCallback);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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