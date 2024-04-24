import airsim
import cv2

client = airsim.VehicleClient()
cv2.imwrite('cube_scene.jpg', cv2.imdecode(airsim.string_to_uint8_array(client.simGetImage("0", airsim.ImageType.CubeScene)), cv2.IMREAD_COLOR))
cv2.imwrite('cube_segmentation.jpg', cv2.imdecode(airsim.string_to_uint8_array(client.simGetImage("0", airsim.ImageType.CubeScene)), cv2.IMREAD_COLOR))