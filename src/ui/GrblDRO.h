#include "DRO.h"

class GrblDRO : public DRO {

public:
    GrblDRO() : cMode{Mode::AXES} {}
    void begin() override ;

protected:
    
    void drawContents() override;

    void onButton(int bt, int8_t arg) override;

private:
    enum class Mode {
        AXES, SPINDLE
    };
    Mode cMode;
};