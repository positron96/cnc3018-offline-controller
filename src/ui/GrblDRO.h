#include "DRO.h"

class GrblDRO : public DRO {

public:
    void begin() override ;

protected:
    
    void drawContents() override;
    
};