#include "FileChooser.h"

#include "../debug.h"

constexpr size_t FileChooser::MAX_FILES;
constexpr size_t FileChooser::VISIBLE_FILES;

void FileChooser::begin() {
}

void FileChooser::onShow() {
    haveCard = SD.begin(PIN_CD); //todo can be done better.
    // on show show, on begin init
    if (!haveCard) {
        LOGF("SD failed\n");
        return;
    }
    loadDirContents(SD.open("/"));
}

bool FileChooser::isGCode(const String &s) {
    int p = s.lastIndexOf('.');
    if (p == -1)
        return true; // files without extension can be printed
    String ext = s.substring(p + 1);
    ext.toLowerCase();
    return (ext == "nc" ||
            ext == "gc" ||
            ext == "gco" ||
            ext == "txt" ||
            ext == "gcode");
}

void FileChooser::loadDirContents(File newDir, int startingIndex) {
    if (!newDir) return;
    LOGF("load dir %s\n", newDir.name());
    if (!cDir || strcmp(cDir.name(), newDir.name()) != 0) {
        cDir = newDir;
    }
    topLine = 0;
    selLine = 0;
    File file;
    files.clear();
    cDir.rewindDirectory();
    int i = 0;
    while ((file = cDir.openNextFile())) {
        if (i >= startingIndex && i < startingIndex + (int) MAX_FILES) {
            String name = file.name();
            LOGF("loadDirContents: file %s\n", name.c_str());
            int p = name.lastIndexOf('/');
            if (p >= 0) name = name.substring(p + 1);

            if (file.isDirectory()) {
                name += "/";
                files.push_back(name);
            } else {
                if (isGCode(name)) { files.push_back(name); }
            }

            if (i == MAX_FILES) break;
        }
        i++;
        file.close();
    }

    LOGF("file count %d\n", files.size());
    setDirty();
}

void FileChooser::drawContents() {
    U8G2 &u8g2 = Display::u8g2;
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_nokiafc22_tr);

    int y0 = Display::STATUS_BAR_HEIGHT / 2,
            y = Display::STATUS_BAR_HEIGHT,
            h = 10;

    if (!haveCard) {
        u8g2.drawStr(2, y0, "NO CARD");
        u8g2.drawStr(2, y, "Press OK to refresh");
        return;
    }

    const char *t = cDir.name();

    u8g2.drawStr(2, y0, t);

    const int visibleLines = min(VISIBLE_FILES, files.size() - topLine);
    for (int i = 0; i < visibleLines; i++) {
        if (i + topLine == selLine) {
            u8g2.setDrawColor(1);
            u8g2.drawBox(0, y, u8g2.getWidth(), h);
            u8g2.setDrawColor(0);
        } else u8g2.setDrawColor(1);

        u8g2.drawStr(2, y, files[topLine + i].c_str());
        y += h;
    }
}


void FileChooser::onButton(int bt, Evt evt) {
    if (evt != Evt::DOWN) return;

    if (!haveCard) {
        if (bt == Display::BT_CENTER) {
            onShow();
            setDirty();
        } else if (bt == Display::BT_L) {
            returnCallback(false, nullptr);
        }
        return;
    }
    switch (bt) {
        case Display::BT_UP:
            if (selLine > 0) {
                selLine--;
                if (selLine < topLine) {
                    topLine -= VISIBLE_FILES - 1;
                    if (topLine < 0)topLine = 0;
                }
                setDirty();
            }
            break;
        case Display::BT_DOWN:
            if (selLine < files.size() - 1) {
                selLine++;
                if (selLine >= topLine + VISIBLE_FILES) topLine += VISIBLE_FILES - 1;
                setDirty();
            }
            break;
        case Display::BT_L: {
            String newPath = cDir.name();

            if (trail.empty()) {
                LOGF("onButtonPressed(BT2): quit\n");
                if (returnCallback)
                    returnCallback(false, "");
            } else {
                LOGF("onButtonPressed(BT2): moving up from %s\n", newPath.c_str());
                trail.pop_back();
                newPath = currentPath();
                LOGF("onButtonPressed(BT2): moving to %s\n", newPath.c_str());
                // int p = newPath.lastIndexOf("/");
                // if(p==-1) newPath="../"; else
                // if(p==0) newPath="/"; else newPath = newPath.substring(0, p);
                loadDirContents(SD.open(newPath));
            }
            break;
        }
        case Display::BT_R:
        case Display::BT_CENTER: {
            String file = files[selLine];
            LOGF("onButtonPressed(BT1): cDir='%s'  file='%s'\n", cDir.name(), file.c_str());
            bool isDir = file.charAt(file.length() - 1) == '/';
            if (isDir) {
                file = file.substring(0, file.length() - 1);
            }
            //String cDirName = cDir.name();
            //if(cDirName.charAt(cDirName.length()-1) != '/' ) cDirName+="/";
            //String newPath = cDirName+file;
            if (isDir) {
                LOGF("cdir is %s, file is %s\n", cDir.name(), file.c_str());
                trail.push_back(file);
                loadDirContents(SD.open(currentPath()), 0);
            } else {
                if (returnCallback) {
                    String c = currentPath() + "/" + file;
                    returnCallback(true, c.c_str());
                }
            }
            break;
        }
        default:
            break;
    }

}

String FileChooser::currentPath() {
    String ret;
    for (const String &p: trail) {
        ret += "/" + p;
    }
    LOGF("currentPath = %s\n", ret.c_str());
    return ret;
}
