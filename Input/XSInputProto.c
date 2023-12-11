#include <stdio.h>
#include <libudev.h>

int main() 
{
    struct udev *udev;
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *entry;
    struct udev_device *dev;

    // Create a udev object
    udev = udev_new();
    if (!udev) {
        fprintf(stderr, "Failed to create udev context\n");
        return 1;
    }

    // Create an enumerate object to scan devices
    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "input");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);

    // Iterate over the list of input devices
    udev_list_entry_foreach(entry, devices) 
    {
        const char *path = udev_list_entry_get_name(entry);

        // Get the udev device for the device path
        dev = udev_device_new_from_syspath(udev, path);

        // Check for the ID_INPUT_MOUSE and ID_INPUT_KEYBOARD attributes
        const char *mouse = udev_device_get_property_value(dev, "ID_INPUT_MOUSE");
        const char *keyboard = udev_device_get_property_value(dev, "ID_INPUT_KEYBOARD");

        printf("Device: %s\n", udev_device_get_devnode(dev));
        printf("I: DEVTYPE=%s\n", udev_device_get_devtype(dev));
        printf("ID_INPUT_MOUSE: %s\n", mouse ? "Yes" : "No");
        printf("ID_INPUT_KEYBOARD: %s\n\n", keyboard ? "Yes" : "No");

        udev_device_unref(dev);
    }

    // Cleanup and release resources
    udev_enumerate_unref(enumerate);
    udev_unref(udev);

    return 0;
}





