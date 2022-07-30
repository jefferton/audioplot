#include "audioplot_pfd.h"

#include "portable-file-dialogs.h"

#include <string>
#include <vector>

std::string promptForFilename()
{
    // Select file to load
    pfd::open_file ofd = pfd::open_file("Choose Data File",
                                        "",                       // default_path
                                        { "Supported Files (.wav, .mp3, .ogg)",
                                          "*.wav *.mp3 *.ogg" }); // filters
    std::vector<std::string> ofdResult = ofd.result();
    if (ofdResult.size() != 1) {
        std::cout << "No File Selected\n";
        return "";
    }
    else {
        return ofdResult[0];
    }
}