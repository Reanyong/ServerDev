// controllers/TestController.h
#pragma once

#include "BaseController.h"

class TestController : public BaseController {
public:
    ApiResponse handleGet(const std::string& path, const std::string& query = "") override;
    ApiResponse handlePost(const std::string& path, const std::string& body = "") override;
    ApiResponse handlePut(const std::string& path, const std::string& body = "") override;
    ApiResponse handleDelete(const std::string& path, const std::string& query = "") override;
};
