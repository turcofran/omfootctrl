IMPORTANT:
Use qv4l2 to change the camera setting in order to get 30fps.

      LE CAMBIE CON EL QV4L2 EL EXPOSURE, AUTO a aperture priority mode 
      Y EL EXPOSURE, AUTO PRIORITY destildado, y anda a 30 fps!
---------------------------------------------------------------------------------

Some options to get information about the camera, its modes, etc, using v4l2-ctl
$ v4l2-ctl --list-devices
Lenovo EasyCamera (usb-0000:00:14.0-4):
	/dev/video0


$ v4l2-ctl --list-formats-ext
ioctl: VIDIOC_ENUM_FMT
	Index       : 0
	Type        : Video Capture
	Pixel Format: 'MJPG' (compressed)
	Name        : MJPEG
		Size: Discrete 1280x720
			Interval: Discrete 0.033s (30.000 fps)
		Size: Discrete 160x120
			Interval: Discrete 0.033s (30.000 fps)
		Size: Discrete 320x240
			Interval: Discrete 0.033s (30.000 fps)
		Size: Discrete 640x480
			Interval: Discrete 0.033s (30.000 fps)
		Size: Discrete 800x600
			Interval: Discrete 0.033s (30.000 fps)

	Index       : 1
	Type        : Video Capture
	Pixel Format: 'YUYV'
	Name        : YUV 4:2:2 (YUYV)
		Size: Discrete 640x480
			Interval: Discrete 0.033s (30.000 fps)
		Size: Discrete 160x120
			Interval: Discrete 0.033s (30.000 fps)
		Size: Discrete 320x240
			Interval: Discrete 0.033s (30.000 fps)
		Size: Discrete 800x600
			Interval: Discrete 0.125s (8.000 fps)
		Size: Discrete 1280x720
			Interval: Discrete 0.125s (8.000 fps)
      

$ v4l2-ctl -L    <<<<----- Available commands!!!
                     brightness (int)    : min=-64 max=64 step=1 default=0 value=0
                       contrast (int)    : min=0 max=100 step=1 default=50 value=50
                     saturation (int)    : min=0 max=100 step=1 default=64 value=64
                            hue (int)    : min=-180 max=180 step=1 default=0 value=0
 white_balance_temperature_auto (bool)   : default=1 value=1
                          gamma (int)    : min=100 max=500 step=1 default=300 value=300
           power_line_frequency (menu)   : min=0 max=2 default=1 value=1
				0: Disabled
				1: 50 Hz
				2: 60 Hz
      white_balance_temperature (int)    : min=2800 max=6500 step=10 default=4600 value=4600 flags=inactive
                      sharpness (int)    : min=0 max=100 step=1 default=50 value=50
         backlight_compensation (int)    : min=0 max=1 step=1 default=0 value=0
                  exposure_auto (menu)   : min=0 max=3 default=3 value=3
				1: Manual Mode
				3: Aperture Priority Mode
         exposure_auto_priority (bool)   : default=0 value=0
                     brightness (int)    : min=-64 max=64 step=1 default=0 value=0
                       contrast (int)    : min=0 max=100 step=1 default=50 value=50
                     saturation (int)    : min=0 max=100 step=1 default=64 value=64
                            hue (int)    : min=-180 max=180 step=1 default=0 value=0
 white_balance_temperature_auto (bool)   : default=1 value=1
                          gamma (int)    : min=100 max=500 step=1 default=300 value=300
           power_line_frequency (menu)   : min=0 max=2 default=1 value=1
				0: Disabled
				1: 50 Hz
				2: 60 Hz
      white_balance_temperature (int)    : min=2800 max=6500 step=10 default=4600 value=4600 flags=inactive
                      sharpness (int)    : min=0 max=100 step=1 default=50 value=50
         backlight_compensation (int)    : min=0 max=1 step=1 default=0 value=0


UVCVIDEO DRIVER
---------------
$ v4l2-ctl -D   <<<<----- Driver information!
Driver Info (not using libv4l2):
	Driver name   : uvcvideo
	Card type     : Lenovo EasyCamera
	Bus info      : usb-0000:00:14.0-4
	Driver version: 3.16.7
	Capabilities  : 0x84000001
		Video Capture
		Streaming
		Device Capabilities
	Device Caps   : 0x04000001
		Video Capture
		Streaming
The driver uvcvideo is the driver for USB Video Class, and it provides support for any device which complains the UVC. 
The driver implements the Video4Linux 2 (V4L2) API.
We can find more info about the http://www.ideasonboard.org/uvc/.
There, it sais that has support for this camera or a similar one:
04f2:b105 	Lenovo EasyCamera (Lenovo IdeaPad Y530 notebooks) 	Chicony Electronics
Here, the driver info is:
# modinfo uvcvideo
filename:       /lib/modules/3.16.0-4-amd64/kernel/drivers/media/usb/uvc/uvcvideo.ko
version:        1.1.1
license:        GPL
description:    USB Video Class driver
author:         Laurent Pinchart <laurent.pinchart@ideasonboard.com>
srcversion:     C27C0860752025E89CBA9D1
alias:          usb:v*p*d*dc*dsc*dp*ic0Eisc01ip00in*
...
alias:          usb:v0416pA91Ad*dc*dsc*dp*ic0Eisc01ip00in*
depends:        videodev,videobuf2-core,usbcore,media,videobuf2-vmalloc
intree:         Y
vermagic:       3.16.0-4-amd64 SMP mod_unload modversions 
parm:           clock:Video buffers timestamp clock
parm:           hwtimestamps:Use hardware timestamps (uint)
parm:           nodrop:Don't drop incomplete frames (uint)
parm:           quirks:Forced device quirks (uint)
parm:           trace:Trace level bitmask (uint)
parm:           timeout:Streaming control requests timeout (uint)
It is possible to pass arguments to the driver, and also set default config for it. On the link http://www.ideasonboard.org/uvc/faq/ it is possible to find more info.


$ v4l2-ctl --all    <<<<------ All the information available for the camera
Driver Info (not using libv4l2):
	Driver name   : uvcvideo
	Card type     : Lenovo EasyCamera
	Bus info      : usb-0000:00:14.0-4
	Driver version: 3.16.7
	Capabilities  : 0x84000001
		Video Capture
		Streaming
		Device Capabilities
	Device Caps   : 0x04000001
		Video Capture
		Streaming
Priority: 2
Video input : 0 (Camera 1: ok)
Format Video Capture:
	Width/Height  : 320/240
	Pixel Format  : 'YUYV'
	Field         : None
	Bytes per Line: 640
	Size Image    : 153600
	Colorspace    : SRGB
Crop Capability Video Capture:
	Bounds      : Left 0, Top 0, Width 320, Height 240
	Default     : Left 0, Top 0, Width 320, Height 240
	Pixel Aspect: 1/1
Streaming Parameters Video Capture:
	Capabilities     : timeperframe
	Frames per second: 30.000 (30/1)
	Read buffers     : 0
                     brightness (int)    : min=-64 max=64 step=1 default=0 value=0
                       contrast (int)    : min=0 max=100 step=1 default=50 value=50
                     saturation (int)    : min=0 max=100 step=1 default=64 value=64
                            hue (int)    : min=-180 max=180 step=1 default=0 value=0
 white_balance_temperature_auto (bool)   : default=1 value=1
                          gamma (int)    : min=100 max=500 step=1 default=300 value=300
           power_line_frequency (menu)   : min=0 max=2 default=1 value=1
      white_balance_temperature (int)    : min=2800 max=6500 step=10 default=4600 value=4600 flags=inactive
                      sharpness (int)    : min=0 max=100 step=1 default=50 value=50
         backlight_compensation (int)    : min=0 max=1 step=1 default=0 value=0
                  exposure_auto (menu)   : min=0 max=3 default=3 value=3
         exposure_auto_priority (bool)   : default=0 value=0
                     brightness (int)    : min=-64 max=64 step=1 default=0 value=0
                       contrast (int)    : min=0 max=100 step=1 default=50 value=50
                     saturation (int)    : min=0 max=100 step=1 default=64 value=64
                            hue (int)    : min=-180 max=180 step=1 default=0 value=0
 white_balance_temperature_auto (bool)   : default=1 value=1
                          gamma (int)    : min=100 max=500 step=1 default=300 value=300
           power_line_frequency (menu)   : min=0 max=2 default=1 value=1
      white_balance_temperature (int)    : min=2800 max=6500 step=10 default=4600 value=4600 flags=inactive
                      sharpness (int)    : min=0 max=100 step=1 default=50 value=50
         backlight_compensation (int)    : min=0 max=1 step=1 default=0 value=0


MODIFY PARAMETERS
------------------
Some parameters access is not implemented on OpenCV, at least on 2.4. So, a way would be by using through the lib libv4l2.h.
http://stackoverflow.com/questions/15035420/configuring-camera-properties-in-new-ocv-2-4-3
https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#camera-controls
So, I try to modify the time using the V4L2_CID_EXPOSURE_ABSOLUTE, but is not success. That is because the camera doesn't implement that control, as the cmd v4l2-ctl --all says.
So, the solution for this camera is set the control as AUTO, but without auto priotitym which means that the frame rate must remain constant.
       

OPENCV
------
The VideoCapture::read() method includes other two methods, the grab() and release().


FPS
---
With this camera (and maybe with this kind of sensors), the maximum FPS is 30fps, 33ms. And the delay of the read method is that... 


TODO:
----
Consider cameras with grather FPS:
http://www.tomshardware.co.uk/forum/236517-50-good-webcam-linux
One example is the Phillips Model SPC 900NC with has 90 fps at VGA, but seems not cheap. It uses the pwc driver. Ebay used at $149.99!

Automate the camera setting. Driver conf? UDEV rule? 

DEBUG flag via cmake? USE_WIN?
