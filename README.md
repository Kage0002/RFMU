# RFMU

This repository contains a Qt plugin and loader for the E6300 hardware.

## Selecting Visible Modules

At runtime you can restrict which analyzer modules are visible by setting the
`RFMU_FEATURE_MODE` environment variable before launching the plugin or loader.

```
# available options
#  SG - show only the signal source
#  SA - show only the spectrum analyzer
#  NA - show only the network analyzer
#  ALL - show all modules (default)
export RFMU_FEATURE_MODE=SA
./MyPluginLoader/MyPluginLoader
```

If the variable is not set, the plugin falls back to a build-time default.
Select this default by adding one of
`DEFAULT_RFMU_FEATURE_MODE_SG`, `DEFAULT_RFMU_FEATURE_MODE_SA`,
`DEFAULT_RFMU_FEATURE_MODE_NA`, or `DEFAULT_RFMU_FEATURE_MODE_ALL`
to the `DEFINES` line in `E6300TestPlugin.pro`.
