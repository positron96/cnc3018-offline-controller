#pragma once
#include <SD.h>
#include <functional>
#include <etl/vector.h>

#include "Screen.h"

using CallbackFn = std::function<void(bool, const char*)>;

class FileChooser: public Screen {
public:

    void begin() override;    

    void onShow() override;

    void setCallback(const CallbackFn &cb) {
        returnCallback = cb;
    }
protected:

    void drawContents() override;

    void onButton(int bt, Evt evt) override;


private:
    CallbackFn returnCallback;
    static constexpr size_t MAX_FILES = 50;
    static constexpr size_t VISIBLE_FILES = 4;
    bool haveCard;
    size_t selLine;
    size_t topLine;
    File cDir;
    etl::vector<String, MAX_FILES> files;
    etl::vector<String, 5> trail;

    void loadDirContents(File newDir, int startingIndex=0) ;

    bool isGCode(const String &s);

    String currentPath();

};