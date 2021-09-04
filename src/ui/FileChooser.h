#pragma once

#include "Screen.h"

#include <SD.h>
#include <functional>
#include <etl/vector.h>


class FileChooser: public Screen {
public:

    void begin() override;
    

    //void loop() override {    }

    void setCallback(const std::function<void(bool, String)> &cb) {
        returnCallback = cb;
    }

private:
    std::function<void(bool, String)> returnCallback;
    bool haveCard;
    size_t selLine;
    size_t topLine;
    File cDir;
    static const size_t MAX_FILES = 50;
    static const size_t VISIBLE_FILES = 4;
    //String files[MAX_FILES];
    etl::vector<String, MAX_FILES> files;
    etl::vector<String, 5> trail;

    void loadDirContents(File newDir, int startingIndex=0) ;

    bool isGCode(const String &s);

    String currentPath();

protected:

    void drawContents() override;

    void onButton(int bt, Evt evt) override;

};