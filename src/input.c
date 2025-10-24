#include "input.h"
#include <string.h>
#include <psp2/kernel/processmgr.h>
#include <vita2d.h>

void utf16_to_utf8(const uint16_t* src, char* dst, size_t dst_size) {
    size_t i;
    for (i = 0; i < dst_size - 1 && src[i] != 0; i++) {
        dst[i] = (char)(src[i] & 0xFF);
    }
    dst[i] = '\0';
}

int kb(const uint16_t* title, uint16_t* initial_text, uint16_t* output_buffer, int password_mode) {
    SceCommonDialogConfigParam config;
    sceCommonDialogConfigParamInit(&config);
    sceCommonDialogSetConfigParam(&config);

    SceImeDialogParam param;
    sceImeDialogParamInit(&param);
    param.supportedLanguages = SCE_IME_LANGUAGE_ENGLISH;
    param.languagesForced = SCE_TRUE;
    param.type = SCE_IME_TYPE_DEFAULT;
    param.option = 0;
    param.textBoxMode = password_mode ? SCE_IME_DIALOG_TEXTBOX_MODE_PASSWORD : SCE_IME_DIALOG_TEXTBOX_MODE_DEFAULT;
    param.title = title;
    param.maxTextLength = MAX_TEXT_LENGTH;
    param.initialText = initial_text;
    param.inputTextBuffer = output_buffer;

    sceImeDialogInit(&param);

    int result = 0;
    int done = 0;
    
    while (!done) {
        SceCommonDialogUpdateParam update_param;
        memset(&update_param, 0, sizeof(update_param));
        update_param.renderTarget.colorSurfaceData = vita2d_get_current_fb();
        update_param.renderTarget.surfaceType = SCE_GXM_COLOR_SURFACE_LINEAR;
        update_param.renderTarget.colorFormat = SCE_GXM_COLOR_FORMAT_A8B8G8R8;
        update_param.renderTarget.width = 960;
        update_param.renderTarget.height = 544;
        update_param.renderTarget.strideInPixels = 960;

        sceCommonDialogUpdate(&update_param);

        if (sceImeDialogGetStatus() == SCE_COMMON_DIALOG_STATUS_FINISHED) {
            SceImeDialogResult ime_result;
            memset(&ime_result, 0, sizeof(ime_result));
            sceImeDialogGetResult(&ime_result);
            
            if (ime_result.button == SCE_IME_DIALOG_BUTTON_ENTER) {
                result = 1;
            } else {
                result = 0;
            }
            sceImeDialogTerm();
            done = 1;
        }
        
        sceKernelDelayThread(10000);
    }
    
    return result;
}
