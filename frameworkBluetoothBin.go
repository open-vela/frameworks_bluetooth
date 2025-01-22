package frameworkBluetooth

import (
        "android/soong/android"
        "android/soong/cc"
)

func init() {
    android.RegisterModuleType("frameworkBluetooth_cc_binary", frameworkBluetoothBinDefaultsFactory)
}

func frameworkBluetoothBinDefaultsFactory() (android.Module) {
    module := cc.BinaryFactory()
    android.AddLoadHook(module, frameworkBluetoothBinHook)
    return module
}

func frameworkBluetoothBinHook(ctx android.LoadHookContext) {
    //AConfig() function is at build/soong/android/config.go

    Version := ctx.AConfig().PlatformVersionName()

    type props struct {
        Srcs []string
        Static_libs []string
        Shared_libs []string
        Cflags []string
    }

    p := &props{}

    if (Version == "12") {
        p.Cflags = append(p.Cflags, "-DANDROID_12")
    } else if (Version == "14") {
        p.Cflags = append(p.Cflags, "-DANDROID_14")
    }

    ctx.AppendProperties(p)
}
