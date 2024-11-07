#include <vector>
#include "ap_scan_result.h"

class ApScanResults {
public:
    uint8_t totalAccessPointsFound;
    std::vector<ApScanResult> accessPoints;
};