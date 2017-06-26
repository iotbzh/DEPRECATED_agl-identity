# agl-identity

## Binding

This binding provide the following API:
* `login`: Try to login the specified identity.
* `logout`: Try to logout the specified identity.
* `open_session`: Try to authenticate an identity using PAM.
* `close_session`: Try to close an opened session.
* `set_data`: Store a json for an identity/application pair.
* `get_data`: Get a json for an identity/application pair.

# PAM's module

This binding make use of PAM to authenticate a user.
A sample PAM module is provided, but you can write your own to allow different authentication workflows.

The sample module assume there is a `identity.json` file located at the root of an usb-stick. When the user plug-in the key, an udev's rule notify the binding and make a call to the `pam_auth` verb. The binding will then call the PAM module to authenticate the user. The sample PAM module will mount and read the file and open a session if the identity is valid.

When the user unplug the usb-stick, udev will notify the binding which will close the session.

# Udev's rules

The sample PAM module work with usb-stick. In order to detect plug and unplug action, some udev's rules are required.
