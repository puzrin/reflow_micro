User manual
===========

You control everything through the web app. At the moment, Web Bluetooth works
only in Chrome, Edge, and other Chromium-based browsers, so iPhone and iPad
owners should open the app in a desktop browser to access every feature. The
UI is intentionally simple; the short notes below highlight a few useful tricks.

NOTE: Remember to [calibrate](./calibration.md) each new head before use.

## Install the web app as a "program"

The control page can be saved as a Progressive Web App (PWA), so it behaves
like an installed program. This step is optional—you can always keep using the
usual browser tab—but the shortcut gives one-tap access, opens full-screen, and
keeps working even when the internet connection drops.

### Mobile (Android)

1. Open the web app in Chrome on your Android phone or tablet.
2. Tap **Add to Home Screen** when the banner appears or from the browser menu.
3. Confirm the name (for example, "Reflow Controller") and tap **Add**. The
   home screen icon appears; it opens the app full-screen and keeps running
   even offline. Give it a few seconds—if the icon does not show up right away,
   check the full app list.
4. To remove the shortcut, long-press the icon and pick **Remove/Delete App**.
   Follow the Add to Home Screen flow again if you want it back.

> Safari on iOS still cannot talk over Bluetooth, so use the desktop
> instructions below to control the device from a Mac or PC.

### Desktop (Chrome / Edge / Chromium)

1. Open the web app in Chrome, Edge, or any Chromium-based browser on Windows,
   macOS or Linux.
2. Click the install icon in the address bar (a little monitor or plus sign) or
   open the browser menu and choose **Install App**.
3. Confirm the prompt. The shortcut lands in your apps list, and you can pin it
   to the taskbar or dock.
4. To uninstall, right-click the app icon and choose **Uninstall**, or visit
   **chrome://apps** or **edge://apps** and remove it from there.


## Button control

- **Long-press (about 2 s)** – starts the temperature profile selected in the
  web app.
- **One short press** – stops the run if a profile is active.
- **Five quick presses** – puts the device into Bluetooth pairing mode; do this
  when the web app asks you.


## LED color

- **Green** – idle and ready to start.
- **Yellow → Red → White** – a profile is running; the color shows the
  hotplate temperature (hot / very hot / extremely hot).
- **Blinking blue** – Bluetooth pairing/authentication mode.
