#pragma once

#include "model.h"

class Controller {
 public:
  Controller(Model& current);
  bool needRefresh() const { return needRefresh_; }

 private:
  const char* dataFilePath = "/last-displayed.json";
  Model& current_;
  bool needRefresh_ = true;

  void writeData();
};
