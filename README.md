# Welcome to Colosseum, a successor of [AirSim](https://github.com/microsoft/AirSim)
  
## Build Status
[![Ubuntu Build](https://github.com/CodexLabsLLC/Colosseum/actions/workflows/test_ubuntu.yml/badge.svg)](https://github.com/CodexLabsLLC/Colosseum/actions/workflows/test_ubuntu.yml)
[![MacOS Build](https://github.com/CodexLabsLLC/Colosseum/actions/workflows/test_macos.yml/badge.svg)](https://github.com/CodexLabsLLC/Colosseum/actions/workflows/test_macos.yml)
[![Windows Build](https://github.com/CodexLabsLLC/Colosseum/actions/workflows/test_windows.yml/badge.svg)](https://github.com/CodexLabsLLC/Colosseum/actions/workflows/test_windows.yml)

[![](https://dcbadge.vercel.app/api/server/y9ZJKKKn8J)](https://discord.gg/y9ZJKKKn8J)
  
## Looking for more performance?
The company managing this repo created the SWARM Developer System to help build, simulate and deploy single and
multi-agent autonomous systems. Check it out here: [SWARM Developer System](https://www.swarmsim.io/overview/developer)
  
## IMPORTANT ANNOUNCEMENT
Moving forward, we are now using Unreal Engine 5 version 5.03 or greater! If you
want to use UE4.27, you can use the branch `ue4.27`.
  
## Unreal Engine Version for Main Branch
The main branch of this repository **only** supports Unreal Engine 5.2! Please see our other branches
for other versions that we support.
  
## Currently Supported Operating Systems
Below are the list of officially supported Operating Systems, with full Unreal Engine support:
### Windows
- Windows 10 (Latest)

### Linux
- ~~Ubuntu 18.04~~ (NO LONGER SUPPORTED. 18.04 is EOL so we will not be checking this anymore and GitHub doesn't support CI builds)
- Ubuntu 20.04
  
**NOTE** Ubuntu 22.04 is not currently supported due to Vulkan support. If this changes, we will notify you here. If you want to use Colosseum on 22.04, we highly recommend that you use Docker.

### MacOS (Non-M1 Macs only)
- MacOS Monterey (12)
- MacOS (11)
  
**NOTE** MacOS support is highly experimental and may be dropped in future releases. This is because Apple continually changes their build tools and doesn't like 3rd party developers in general. There are ongoing discussions to remove this support.

## Sponsors
1. Codex Laboratories LLC [Website](https://www.codex-labs-llc.com)
  
## Introduction
  
Colosseum is a simulator for robotic, autonomous systems, built on [Unreal Engine](https://www.unrealengine.com/) (we now also have an experimental [Unity](https://unity3d.com/) release). It is open-source, cross platform, and supports software-in-the-loop simulation with popular flight controllers such as PX4 & ArduPilot and hardware-in-loop with PX4 for physically and visually realistic simulations. It is developed as an Unreal plugin that can simply be dropped into any Unreal environment. Similarly, we have an experimental release for a Unity plugin.
  
This is a fork of the AirSim repository, which Microsoft decided to shutdown in July of 2022. This fork serves as a waypoint to building a new and better simulation platform. The creater and maintainer of this fork is Codex Laboratories LLC (our website is [here](https://www.codex-labs-llc.com)). Colosseum is one of the underlying simulation systems that we use in our product, the [SWARM Simulation Platform](https://www.swarmsim.io). This platform exists to provide pre-built tools and low-code/no-code autonomy solutions. Please feel free to check this platform out and reach out if interested.

## Join the Community
We have decided to create a Discord channel to better allow for community engagement. Join here: [Colosseum Robotics Discord](https://discord.gg/y9ZJKKKn8J).
  
  
## Goals and Project Development
This section will contain a list of the current features that the community and Codex Labs are working on to support and build.

Click [here](https://docs.google.com/document/d/1doohQTos4v1tg4Wv6SliQFnKNK1MouKX2efg2mapXFU/edit?usp=sharing) to view our current development goals!

If you want to be apart of the official development team, attend meetings, etc., please utilize the Slack channel (link above) and 
let Tyler Fedrizzi know!

## License

This project is released under the MIT License. Please review the [License file](LICENSE) for more details.


## Panorama and Fisheye
### Indroduction
Add new image type to Airsim.

Refer to [this page](http://www.huyaoyu.com/technical/2021/04/29/modify-airsim.html) to rewrite the airsim plugin.

In addition, Unreal Engine's **USceneCaptureComponentCube** does not provide post-processing, this function must be added manually to capture Segmentation images.

### Requirement
* UE5.2 release-version: Please install UE5.2 release-version from [this page](https://github.com/EpicGames/UnrealEngine)

### Modify Source Code
**Unrea Engine Source code** : Added post-processing components to USceneCaptureComponentCube, and enable post-processing for USceneCaptureComponentCube.

Engine\Source\Runtime\Engine\Classes\Components\SceneCaptureComponentCube.h : add following code.

```cpp
	UPROPERTY(interp, Category = PostProcessVolume, meta = (ShowOnlyInnerProperties))
	struct FPostProcessSettings PostProcessSettings;

	/** Range (0.0, 1.0) where 0 indicates no effect, 1 indicates full effect. */
	UPROPERTY(interp, Category = PostProcessVolume, BlueprintReadWrite, meta = (UIMin = "0.0", UIMax = "1.0"))
	float PostProcessBlendWeight;
```
Engine\Source\Runtime\Renderer\Private\SceneCaptureRendering.cpp : At line 1145, modify the code as following, add the post-processing component we just add.

```cpp
FSceneRenderer* SceneRenderer = CreateSceneRendererForSceneCapture(
				this, 
				CaptureComponent,
				TextureTarget->GameThread_GetRenderTargetResource(), 
				CaptureSize, 
				ViewRotationMatrix,
				Location, 
				ProjectionMatrix, 
				false, 
				CaptureComponent->MaxViewDistanceOverride,
				bCaptureSceneColor, 
				&CaptureComponent->PostProcessSettings,
				CaptureComponent->PostProcessBlendWeight, 
				CaptureComponent->GetViewOwner(), faceidx);

```

**AirSim Plugin** : Modify the following files, add USceneCaptureComponentCube to the related functions for capturing images.

```
Source/AirLib/include/common/ImageCaptureBase.hpp
Source/AirBlueprintLab.h
Source/AirBlueprintLab.cpp
Source/UnrealImageCapture.cpp
Source/PIPCamra.h
Source/PIPCamea.cpp
Content/Blueprints/BP_PIPCamra.uasset
Source/RenderRequest.h
Source/RenderRequest.cpp
Source/SimHUD/SimHUD.h
Source/SimHUD/SimHUD.cpp
Source/SimHUD/SimHUDWidget.h
Source/DetectionComponent.cpp
Content/Blueprints/BP_SimHUDWidget.uasset
Python client(.\Colosseum\PythonClient\airsim\types.py)
```


### settings.json
Add the following content to settings.json:
```json
{
  "SeeDocsAt": "https://github.com/Microsoft/AirSim/blob/main/docs/settings.md",
  "SettingsVersion": 1.2,
  "//": "SimMode:Car,Multirotor,ComputerVision",
  "SimMode": "Multirotor",
  "LogMessagesVisible": false,  
  "CameraDefaults": {
    "CaptureSettings": [
      {
        "ImageType": 0,
        "Width": 1920,
        "Height": 1080,
        "FOV_Degrees": 90,
        "AutoExposureSpeed": 100,
        "AutoExposureBias": 0,
        "AutoExposureMaxBrightness": 0.64,
        "AutoExposureMinBrightness": 0.03,
        "MotionBlurAmount": 0,
        "TargetGamma": 1.0,
        "ProjectionMode": "",
        "OrthoWidth": 5.12
      },
      {
        "ImageType": 1,
        "Width": 1920,
        "Height": 1080
      },
      {
        "ImageType": 3,
        "Width": 1920,
        "Height": 1080
      },
      {
        "ImageType": 5,
        "Width": 1920,
        "Height": 1080
      },
      {
        "ImageType": 10,
        "Width": 960,
        "Height": 960
      },
      {
        "ImageType": 11,
        "Width": 960,
        "Height": 960
      }
    ]
  },
  "PawnPaths": {
    "BareboneCar": {"PawnBP": "Class'/AirSim/VehicleAdv/Vehicle/VehicleAdvPawn.VehicleAdvPawn_C'"},
    "DefaultCar": {"PawnBP": "Class'/AirSim/VehicleAdv/SUV/SuvCarPawn.SuvCarPawn_C'"},
    "DefaultQuadrotor": {"PawnBP": "Class'/AirSim/Blueprints/BP_FlyingPawn.BP_FlyingPawn_C'"},
    "DefaultComputerVision": {"PawnBP": "Class'/AirSim/Blueprints/BP_ComputerVisionPawn.BP_ComputerVisionPawn_C'"}

  }
}

```
### AirSim API
New image type: CubeScene, CubeSegmentation

If you want to get panorama image by airsim API, you can use following code to get the image:
```python
import airsim
import cv2

client = airsim.VehicleClient()
client.confirmConnection()

cv2.imwrite('cube_scene.jpg', cv2.imdecode(airsim.string_to_uint8_array(client.simGetImage("0", airsim.ImageType.CubeScene)), cv2.IMREAD_COLOR))
cv2.imwrite('cube_segmentation.jpg', cv2.imdecode(airsim.string_to_uint8_array(client.simGetImage("0", airsim.ImageType.CubeScene)), cv2.IMREAD_COLOR))
```

or you can run the sample code under the path: **.\Colosseum\PythonClient\fisheye\\**

[**Panorama image**](https://github.com/Kura0913/Colosseum/tree/fisheye/PythonClient/fisheye/fisheye_example.jpg)
