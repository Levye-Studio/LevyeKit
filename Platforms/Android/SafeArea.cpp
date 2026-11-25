#include <android/api-level.h>
#include <android_native_app_glue.h>
#include <jni.h>

#include <Levye/Display/DisplayTypes.hpp>
#include <initializer_list>

extern "C" android_app* GetAndroidApp();

// Permit the native surface to use the short edges containing a display cutout.
// The game still receives the cutout insets separately for UI layout.
extern "C" void LevyeAndroidEnableCutout() {
  if (android_get_device_api_level() < 28)
    return;

  android_app* app = GetAndroidApp();

  if (!app || !app->activity || !app->activity->vm)
    return;

  JNIEnv* env = nullptr;
  bool attached = false;

  if (app->activity->vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
    if (app->activity->vm->AttachCurrentThread(&env, nullptr) != JNI_OK)
      return;
    attached = true;
  }

  jclass activityClass = env->GetObjectClass(app->activity->clazz);
  const jmethodID getWindow =
      activityClass ? env->GetMethodID(activityClass, "getWindow", "()Landroid/view/Window;")
                    : nullptr;
  jobject window = getWindow ? env->CallObjectMethod(app->activity->clazz, getWindow) : nullptr;
  jclass windowClass = window ? env->GetObjectClass(window) : nullptr;
  const jmethodID getAttributes =
      windowClass ? env->GetMethodID(windowClass, "getAttributes",
                                     "()Landroid/view/WindowManager$LayoutParams;")
                  : nullptr;
  jobject attributes = getAttributes ? env->CallObjectMethod(window, getAttributes) : nullptr;
  jclass attributesClass = attributes ? env->GetObjectClass(attributes) : nullptr;
  const jfieldID mode = attributesClass
                            ? env->GetFieldID(attributesClass, "layoutInDisplayCutoutMode", "I")
                            : nullptr;
  const jmethodID setAttributes =
      windowClass ? env->GetMethodID(windowClass, "setAttributes",
                                     "(Landroid/view/WindowManager$LayoutParams;)V")
                  : nullptr;
  if (mode && setAttributes) {
    env->SetIntField(attributes, mode, 1);  // LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES.
    env->CallVoidMethod(window, setAttributes, attributes);
  }

  for (jobject ref :
       {static_cast<jobject>(attributesClass), attributes, static_cast<jobject>(windowClass),
        window, static_cast<jobject>(activityClass)})
    if (ref)
      env->DeleteLocalRef(ref);
  if (env->ExceptionCheck())
    env->ExceptionClear();
  if (attached)
    app->activity->vm->DetachCurrentThread();
}

// WindowInsets are physical window pixels on Android. Cutout insets are
// independent of whether the system bars are currently visible.
extern "C" void LevyeAndroidSafeInsets(Levye::DisplayInsets* outputInsets) {
  if (!outputInsets)
    return;

  *outputInsets = {};

  if (android_get_device_api_level() < 28)
    return;

  android_app* app = GetAndroidApp();

  if (!app || !app->activity || !app->activity->vm)
    return;

  JNIEnv* env = nullptr;
  bool attached = false;

  if (app->activity->vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
    if (app->activity->vm->AttachCurrentThread(&env, nullptr) != JNI_OK)
      return;
    attached = true;
  }

  Levye::DisplayInsets result{};

  jobject window = nullptr, decor = nullptr, insets = nullptr, cutout = nullptr;
  jclass activityClass = env->GetObjectClass(app->activity->clazz);

  if (activityClass) {
    const jmethodID getWindow =
        env->GetMethodID(activityClass, "getWindow", "()Landroid/view/Window;");
    if (getWindow)
      window = env->CallObjectMethod(app->activity->clazz, getWindow);
  }

  jclass windowClass = window ? env->GetObjectClass(window) : nullptr;
  if (windowClass) {
    const jmethodID getDecor =
        env->GetMethodID(windowClass, "getDecorView", "()Landroid/view/View;");
    if (getDecor)
      decor = env->CallObjectMethod(window, getDecor);
  }

  jclass viewClass = decor ? env->GetObjectClass(decor) : nullptr;
  if (viewClass) {
    const jmethodID getInsets =
        env->GetMethodID(viewClass, "getRootWindowInsets", "()Landroid/view/WindowInsets;");
    if (getInsets)
      insets = env->CallObjectMethod(decor, getInsets);
  }

  jclass insetsClass = insets ? env->GetObjectClass(insets) : nullptr;
  if (insetsClass) {
    const jmethodID getCutout =
        env->GetMethodID(insetsClass, "getDisplayCutout", "()Landroid/view/DisplayCutout;");
    if (getCutout)
      cutout = env->CallObjectMethod(insets, getCutout);
  }

  jclass cutoutClass = cutout ? env->GetObjectClass(cutout) : nullptr;
  if (cutoutClass) {
    const char* methods[] = {"getSafeInsetLeft", "getSafeInsetTop", "getSafeInsetRight",
                             "getSafeInsetBottom"};
    float* output[] = {&result.left, &result.top, &result.right, &result.bottom};
    for (int i = 0; i < 4; ++i) {
      const jmethodID method = env->GetMethodID(cutoutClass, methods[i], "()I");
      if (method)
        *output[i] = static_cast<float>(env->CallIntMethod(cutout, method));
    }
  }

  for (jobject ref :
       {static_cast<jobject>(cutoutClass), cutout, static_cast<jobject>(insetsClass), insets,
        static_cast<jobject>(viewClass), decor, static_cast<jobject>(windowClass), window,
        static_cast<jobject>(activityClass)})
    if (ref)
      env->DeleteLocalRef(ref);
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    result = {};
  }

  if (attached)
    app->activity->vm->DetachCurrentThread();
  *outputInsets = result;
}
