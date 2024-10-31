#include "selfdrive/frogpilot/ui/qt/offroad/device_settings.h"

FrogPilotDevicePanel::FrogPilotDevicePanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  const std::vector<std::tuple<QString, QString, QString, QString>> deviceToggles {
    {"DeviceManagement", tr("Device Settings"), tr("Device behavior settings."), "../frogpilot/assets/toggle_icons/icon_device.png"},
    {"DeviceShutdown", tr("Device Shutdown Timer"), tr("Controls how long the device stays on after you stop driving."), ""},
    {"OfflineMode", tr("Disable Internet Requirement"), tr("Allows the device to work without an internet connection."), ""},
    {"IncreaseThermalLimits", tr("Increase Thermal Safety Limit"), tr("Allows the device to run at higher temperatures than recommended."), ""},
    {"LowVoltageShutdown", tr("Low Battery Shutdown Threshold"), tr("Manages the threshold for shutting down the device to protect the car's battery from excessive drain and potential damage."), ""},
    {"NoLogging", tr("Turn Off Data Tracking"), tr("Disables all data tracking to improve privacy."), ""},
    {"NoUploads", tr("Turn Off Data Uploads"), tr("Stops the device from sending any data to the servers."), ""},

    {"ScreenManagement", tr("Screen Settings"), tr("Screen behavior settings."), "../frogpilot/assets/toggle_icons/icon_light.png"},
    {"ScreenBrightness", tr("Screen Brightness (Offroad)"), tr("Controls the screen brightness when you're not driving."), ""},
    {"ScreenBrightnessOnroad", tr("Screen Brightness (Onroad)"), tr("Controls the screen brightness while you're driving."), ""},
    {"ScreenRecorder", tr("Screen Recorder"), tr("Enables a button in the onroad UI to record the screen."), ""},
    {"ScreenTimeout", tr("Screen Timeout (Offroad)"), tr("Controls how long it takes for the screen to turn off when you're not driving."), ""},
    {"ScreenTimeoutOnroad", tr("Screen Timeout (Onroad)"), tr("Controls how long it takes for the screen to turn off while you're driving."), ""}
  };

  for (const auto &[param, title, desc, icon] : deviceToggles) {
    AbstractControl *deviceToggle;

    if (param == "DeviceManagement") {
      FrogPilotParamManageControl *deviceManagementToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(deviceManagementToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        std::set<QString> modifiedDeviceManagementKeys = deviceManagementKeys;

        if (customizationLevel != 2) {
          modifiedDeviceManagementKeys.erase("IncreaseThermalLimits");
          modifiedDeviceManagementKeys.erase("LowVoltageShutdown");
          modifiedDeviceManagementKeys.erase("NoLogging");
          modifiedDeviceManagementKeys.erase("NoUploads");
          modifiedDeviceManagementKeys.erase("OfflineMode");
        }

        showToggles(modifiedDeviceManagementKeys);
      });
      deviceToggle = deviceManagementToggle;
    } else if (param == "DeviceShutdown") {
      std::map<int, QString> shutdownLabels;
      for (int i = 0; i <= 33; ++i) {
        shutdownLabels[i] = i == 0 ? tr("5 mins") : i <= 3 ? QString::number(i * 15) + tr(" mins") : QString::number(i - 3) + (i == 4 ? tr(" hour") : tr(" hours"));
      }
      deviceToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 33, QString(), shutdownLabels);
    } else if (param == "NoUploads") {
      std::vector<QString> uploadsToggles{"DisableOnroadUploads"};
      std::vector<QString> uploadsToggleNames{tr("Only Onroad")};
      deviceToggle = new FrogPilotButtonToggleControl(param, title, desc, uploadsToggles, uploadsToggleNames);
    } else if (param == "LowVoltageShutdown") {
      deviceToggle = new FrogPilotParamValueControl(param, title, desc, icon, 11.8, 12.5, tr(" volts"), std::map<int, QString>(), 0.01);

    } else if (param == "ScreenManagement") {
      FrogPilotParamManageControl *screenToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(screenToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        std::set<QString> modifiedScreenKeys = screenKeys;

        if (customizationLevel != 2) {
          modifiedScreenKeys.erase("ScreenTimeout");
          modifiedScreenKeys.erase("ScreenTimeoutOnroad");
        }

        showToggles(modifiedScreenKeys);
      });
      deviceToggle = screenToggle;
    } else if (param == "ScreenBrightness" || param == "ScreenBrightnessOnroad") {
      std::map<int, QString> brightnessLabels;
      int minBrightness = (param == "ScreenBrightnessOnroad") ? 0 : 1;
      for (int i = 0; i <= 101; ++i) {
        brightnessLabels[i] = i == 101 ? tr("Auto") : i == 0 ? tr("Screen Off") : QString::number(i) + "%";
      }
      deviceToggle = new FrogPilotParamValueControl(param, title, desc, icon, minBrightness, 101, QString(), brightnessLabels, 1, false, true);
    } else if (param == "ScreenTimeout" || param == "ScreenTimeoutOnroad") {
      deviceToggle = new FrogPilotParamValueControl(param, title, desc, icon, 5, 60, tr(" seconds"));

    } else {
      deviceToggle = new ParamControl(param, title, desc, icon);
    }

    addItem(deviceToggle);
    toggles[param] = deviceToggle;

    makeConnections(deviceToggle);

    if (FrogPilotParamManageControl *frogPilotManageToggle = qobject_cast<FrogPilotParamManageControl*>(deviceToggle)) {
      QObject::connect(frogPilotManageToggle, &FrogPilotParamManageControl::manageButtonClicked, this, &FrogPilotDevicePanel::openParentToggle);
    }

    QObject::connect(deviceToggle, &AbstractControl::showDescriptionEvent, [this]() {
      update();
    });
  }

  QObject::connect(static_cast<ToggleControl*>(toggles["IncreaseThermalLimits"]), &ToggleControl::toggleFlipped, [this](bool state) {
    if (state) {
      FrogPilotConfirmationDialog::toggleAlert(
        tr("WARNING: This can cause premature wear or damage by running the device over comma's recommended temperature limits!"),
        tr("I understand the risks."), this);
    }
  });

  QObject::connect(static_cast<ToggleControl*>(toggles["NoLogging"]), &ToggleControl::toggleFlipped, [this](bool state) {
    if (state) {
      FrogPilotConfirmationDialog::toggleAlert(
        tr("WARNING: This will prevent your drives from being recorded and the data will be unobtainable!"),
        tr("I understand the risks."), this);
    }
  });

  QObject::connect(static_cast<ToggleControl*>(toggles["NoUploads"]), &ToggleControl::toggleFlipped, [this](bool state) {
    if (state) {
      FrogPilotConfirmationDialog::toggleAlert(
        tr("WARNING: This will prevent your drives from appearing on comma connect which may impact debugging and support!"),
        tr("I understand the risks."), this);
    }
  });

  FrogPilotParamValueControl *screenBrightnessToggle = static_cast<FrogPilotParamValueControl*>(toggles["ScreenBrightness"]);
  QObject::connect(screenBrightnessToggle, &FrogPilotParamValueControl::valueChanged, [this](float value) {
    if (!started) {
      uiState()->scene.screen_brightness = value;
    }
  });

  FrogPilotParamValueControl *screenBrightnessOnroadToggle = static_cast<FrogPilotParamValueControl*>(toggles["ScreenBrightnessOnroad"]);
  QObject::connect(screenBrightnessOnroadToggle, &FrogPilotParamValueControl::valueChanged, [this](float value) {
    if (started) {
      uiState()->scene.screen_brightness_onroad = value;
    }
  });

  QObject::connect(parent, &FrogPilotSettingsWindow::closeParentToggle, this, &FrogPilotDevicePanel::hideToggles);
  QObject::connect(uiState(), &UIState::uiUpdate, this, &FrogPilotDevicePanel::updateState);

  hideToggles();
}

void FrogPilotDevicePanel::showEvent(QShowEvent *event) {
  customizationLevel = parent->customizationLevel;
}

void FrogPilotDevicePanel::updateState(const UIState &s) {
  started = s.scene.started;
}

void FrogPilotDevicePanel::showToggles(const std::set<QString> &keys) {
  setUpdatesEnabled(false);

  for (auto &[key, toggle] : toggles) {
    toggle->setVisible(keys.find(key) != keys.end());
  }

  setUpdatesEnabled(true);
  update();
}

void FrogPilotDevicePanel::hideToggles() {
  setUpdatesEnabled(false);

  for (auto &[key, toggle] : toggles) {
    bool subToggles = deviceManagementKeys.find(key) != deviceManagementKeys.end() ||
                      screenKeys.find(key) != screenKeys.end();
    toggle->setVisible(!subToggles);
  }

  setUpdatesEnabled(true);
  update();
}