// controllers/HealthController.h
#pragma once

#include "BaseController.h"

class HealthController : public BaseController {
public:
    ApiResponse handleGet(const std::string& path, const std::string& query = "") override;
};
