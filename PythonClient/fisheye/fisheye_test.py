import airsim
import cv2

client = airsim.VehicleClient()
cv2.imwrite('fisheye.jpg', cv2.imdecode(airsim.string_to_uint8_array(client.simGetImage("0", airsim.ImageType.CubeScene)), cv2.IMREAD_COLOR))