#include <iostream>
#include "RenderRequest.h"
#include "CubemapUnwrapUtils.h"
#include "TextureResource.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Async/TaskGraphInterfaces.h"
#include "ImageUtils.h"

#include "AirBlueprintLib.h"
#include "Async/Async.h"

RenderRequest::RenderRequest(UGameViewportClient* game_viewport, std::function<void()>&& query_camera_pose_cb)
    : params_(nullptr), results_(nullptr), req_size_(0), wait_signal_(new msr::airlib::WorkerThreadSignal), game_viewport_(game_viewport), query_camera_pose_cb_(std::move(query_camera_pose_cb))
{
}

RenderRequest::~RenderRequest()
{
}

// read pixels from render target using render thread, then compress the result into PNG
// argument on the thread that calls this method.
void RenderRequest::getScreenshot(std::shared_ptr<RenderParams> params[], std::vector<std::shared_ptr<RenderResult>>& results, unsigned int req_size, bool use_safe_method)
{
    //TODO: is below really needed?
    for (unsigned int i = 0; i < req_size; ++i) {
        results.push_back(std::make_shared<RenderResult>());

        if (!params[i]->pixels_as_float)
            results[i]->bmp.Reset();
        else
            results[i]->bmp_float.Reset();
        results[i]->time_stamp = 0;
    }

    //make sure we are not on the rendering thread
    CheckNotBlockedOnRenderThread();

    if (use_safe_method) {
        // Cube.
        UE_LOG(LogTemp, Error, TEXT("Cube bets it is not executed under safe method. "));

        for (unsigned int i = 0; i < req_size; ++i) {
            //TODO: below doesn't work right now because it must be running in game thread
            FIntPoint img_size;
            if (!params[i]->pixels_as_float) {
                //below is documented method but more expensive because it forces flush
                FTextureRenderTargetResource* rt_resource = params[i]->render_target->GameThread_GetRenderTargetResource();
                auto flags = setupRenderResource(rt_resource, params[i].get(), results[i].get(), img_size);
                rt_resource->ReadPixels(results[i]->bmp, flags);
            }
            else {
                FTextureRenderTargetResource* rt_resource = params[i]->render_target->GetRenderTargetResource();
                setupRenderResource(rt_resource, params[i].get(), results[i].get(), img_size);
                rt_resource->ReadFloat16Pixels(results[i]->bmp_float);
            }
        }
    }
    else {
        //wait for render thread to pick up our task
        params_ = params;
        results_ = results.data();
        req_size_ = req_size;

        // Queue up the task of querying camera pose in the game thread and synchronizing render thread with camera pose
        AsyncTask(ENamedThreads::GameThread, [this]() {
            check(IsInGameThread());

            saved_DisableWorldRendering_ = game_viewport_->bDisableWorldRendering;
            game_viewport_->bDisableWorldRendering = 0;
            end_draw_handle_ = game_viewport_->OnEndDraw().AddLambda([this] {
                check(IsInGameThread());

                // capture CameraPose for this frame
                query_camera_pose_cb_();

                // The completion is called immeidately after GameThread sends the
                // rendering commands to RenderThread. Hence, our ExecuteTask will
                // execute *immediately* after RenderThread renders the scene!
                RenderRequest* This = this;
                ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)
                (
                    [This](FRHICommandListImmediate& RHICmdList) {
                        This->ExecuteTask();
                    });

                game_viewport_->bDisableWorldRendering = saved_DisableWorldRendering_;

                assert(end_draw_handle_.IsValid());
                game_viewport_->OnEndDraw().Remove(end_draw_handle_);
            });

            // while we're still on GameThread, enqueue request for capture the scene!
            for (unsigned int i = 0; i < req_size_; ++i) {
                auto& temp_param = params_[i];
                if (!temp_param->is_cube) {
                    temp_param->render_component->CaptureSceneDeferred();
                }
                else {
                    // Cube. If render_component only calls the overrided methods, 
                    // we can use polymorphism with virtual function calls.
                    temp_param->render_component_cube->CaptureSceneDeferred();
                    // Get the raw 8bit data.
                    unWarpTextureRenderTargetCube(temp_param->render_target_cube, results_[i]->cube_raw);
                }
            }
        });

        // wait for this task to complete
        while (!wait_signal_->waitFor(5)) {
            // log a message and continue wait
            // lamda function still references a few objects for which there is no refcount.
            // Walking away will cause memory corruption, which is much more difficult to debug.
            UE_LOG(LogTemp, Warning, TEXT("Failed: timeout waiting for screenshot"));
        }
    }

    for (unsigned int i = 0; i < req_size; ++i) {
        if (!params[i]->is_cube) {
            if (!params[i]->pixels_as_float) {
                if (results[i]->width != 0 && results[i]->height != 0) {
                    results[i]->image_data_uint8.SetNumUninitialized(results[i]->width * results[i]->height * 3, false);
                    if (params[i]->compress)
                        UAirBlueprintLib::CompressImageArray(results[i]->width, results[i]->height, results[i]->bmp, results[i]->image_data_uint8);
                    else {
                        uint8* ptr = results[i]->image_data_uint8.GetData();
                        for (const auto& item : results[i]->bmp) {
                            *ptr++ = item.B;
                            *ptr++ = item.G;
                            *ptr++ = item.R;
                        }
                    }
                }
            }
            else {
                results[i]->image_data_float.SetNumUninitialized(results[i]->width * results[i]->height);
                float* ptr = results[i]->image_data_float.GetData();
                UE_LOG(LogTemp, Warning, TEXT("Before copy 2D. "));
                for (const auto& item : results[i]->bmp_float) {
                    *ptr++ = item.R.GetFloat();
                }
                UE_LOG(LogTemp, Warning, TEXT("After copy 2D. "));
            }
        }
        else {
            // params[i]->is_cube
           //// Get the raw 8bit data.
           //unWarpTextureRenderTargetCube(params[i]->render_target_cube, results[i]->cube_raw);

            if (!params[i]->pixels_as_float) {
                // RRG 8bit.
                if (params[i]->compress) {
                    // Compress.
                    compressTArrayAsPng32bit(results[i]->cube_raw, results[i]->cube_image_data,
                        results[i]->width, results[i]->height, 100);
                    // UAirBlueprintLib::CompressImageArray(results[i]->width, results[i]->height, results[i]->bmp, results[i]->image_data_uint8);
                    // Copy the data from TArray64 to TArray.
                    copyFromTArray2TArray(results[i]->cube_image_data, results[i]->image_data_uint8);
                }
                else {
                    copyFromTArray2TArray(results[i]->cube_raw, results[i]->image_data_uint8);
                }
            }
            else {
                // FFloat16Color.
                auto tempF16 = reinterpret_cast<FFloat16*>(results[i]->cube_raw.GetData());

                // Initialize the memory.
                results[i]->image_data_float.SetNumUninitialized(results[i]->width * results[i]->height);

                // Loop and copy.
                float* ptr = results[i]->image_data_float.GetData();
                const int N = results[i]->cube_raw.Num() / 2; // Every FFloat16 contains 2 uint8.
                for (int f16 = 0; f16 < N; f16 += 4) {
                    *ptr++ = (tempF16 + f16)->GetFloat() * 0.01f; // Convert from centimeter to meter.
                }
            }
        }
    } // for i < req_size
}

FReadSurfaceDataFlags RenderRequest::setupRenderResource(const FTextureRenderTargetResource* rt_resource, const RenderParams* params, RenderResult* result, FIntPoint& size)
{
    size = rt_resource->GetSizeXY();
    result->width = size.X;
    result->height = size.Y;
    FReadSurfaceDataFlags flags(RCM_UNorm, CubeFace_MAX);
    flags.SetLinearToGamma(false);

    return flags;
}

bool RenderRequest::unWarpTextureRenderTargetCube(const UTextureRenderTargetCube* TRTCube, TArray64<uint8>& OutData) {
    FIntPoint Size;
    EPixelFormat PixelFormat;

    if (!CubemapHelpers::GenerateLongLatUnwrap(TRTCube, OutData, Size, PixelFormat))
    {
        UE_LOG(LogTemp, Warning, TEXT("CubemapHelpers::GenerateLogLatUnwrap() failed. "));
        return false;
    }

    verifyf(TRTCube->SizeX == Size.Y, TEXT("TRTCube.SizeX = %d, Size.Y = %d"), TRTCube->SizeX, Size.Y);
    verifyf(TRTCube->SizeX == Size.X / 2, TEXT("TRTCube.SizeX = %d, Size.X = %d"), TRTCube->SizeX, Size.X);
    verifyf(TRTCube->GetFormat() == PixelFormat,
        TEXT("TRTCube->GetFormat() = %d, PixelFormat = %d. "), TRTCube->GetFormat(), PixelFormat);

    return true;
}

void RenderRequest::ExecuteTask()
{
    if (params_ != nullptr && req_size_ > 0) {
        for (unsigned int i = 0; i < req_size_; ++i) {
            // Cube.
            if (params_[i]->is_cube) {
                results_[i]->width = params_[i]->render_target_cube->SizeX * 2;
                results_[i]->height = params_[i]->render_target_cube->SizeX;
                results_[i]->time_stamp = msr::airlib::ClockFactory::get()->nowNanos();
                continue;
            }

            FRHICommandListImmediate& RHICmdList = GetImmediateCommandList_ForRenderCommand();
            auto rt_resource = params_[i]->render_target->GetRenderTargetResource();
            if (rt_resource != nullptr) {
                const FTexture2DRHIRef& rhi_texture = rt_resource->GetRenderTargetTexture();
                FIntPoint size;
                auto flags = setupRenderResource(rt_resource, params_[i].get(), results_[i].get(), size);

                //should we be using ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER which was in original commit by @saihv
                //https://github.com/Microsoft/AirSim/pull/162/commits/63e80c43812300a8570b04ed42714a3f6949e63f#diff-56b790f9394f7ca1949ddbb320d8456fR64
                if (!params_[i]->pixels_as_float) {
                    //below is undocumented method that avoids flushing, but it seems to segfault every 2000 or so calls
                    RHICmdList.ReadSurfaceData(
                        rhi_texture,
                        FIntRect(0, 0, size.X, size.Y),
                        results_[i]->bmp,
                        flags);
                }
                else {
                    RHICmdList.ReadSurfaceFloatData(
                        rhi_texture,
                        FIntRect(0, 0, size.X, size.Y),
                        results_[i]->bmp_float,
                        CubeFace_PosX,
                        0,
                        0);
                }
            }

            results_[i]->time_stamp = msr::airlib::ClockFactory::get()->nowNanos();
        }

        req_size_ = 0;
        params_ = nullptr;
        results_ = nullptr;

        wait_signal_->signal();
    }
}
