User manual
===========

You control everything through the web app. At the moment Web Bluetooth works
only in Chrome, Edge and other Chromium-based browsers, so iPhone/iPad owners
should open the app from a desktop browser to use every feature. The UI stays
intentionally simple; the short notes below just highlight a few useful tricks.


## Install web app as "program"

The control page can be saved as a Progressive Web App (PWA), so it behaves
like an installed program. This step is optional—you can always keep using the
usual browser tab—but the shortcut gives one-tap access, opens fullscreen and
keeps working even if the internet drops.

### Mobile (Android)

1. Open the web app site in Chrome on your Android phone or tablet.
2. Tap **Add to Home screen** when the banner appears or via browser menu.
3. Confirm the name (for example "Reflow Controller") and tap **Add**. A home
   screen icon appears; it opens the app fullscreen and keeps running even
   without internet. Give it a few seconds—if the icon does not show up right
   away, look for it in the full app list/drawer.
4. To remove the shortcut, long-press the icon and pick **Remove / Delete App**.
   Repeat the same Add to Home Screen flow if you ever want it back.

> Safari on iOS still cannot talk over Bluetooth, so use the desktop
> instructions below to control it from a Mac or PC.

### Desktop (Chrome / Edge / Chromium)

1. Open the web app in Chrome, Edge or any Chromium-based browser on Windows,
   macOS or Linux.
2. Click the install icon in the address bar (a little monitor or plus sign) or
   open the browser menu → **Install App**.
3. Confirm the prompt. The shortcut lands in your apps list and you can pin it
   to the taskbar or dock.
4. To uninstall, right-click the app icon and choose **Uninstall**, or visit
   **chrome://apps** / **edge://apps** and remove it from there.


## Button control

- **Long press (about 2 s)** – starts whichever temperature profile is selected
  in the web app.
- **One short press** – stops the run if a profile is active.
- **Five quick presses** – puts the device into Bluetooth pairing mode; do this
  when the web app asks you.


## LED color

- **Green** – idle and ready to start.
- **Yellow → Red → White** – a profile is running; the color shows how hot the
  hotplate is (hot / very hot / extremely hot).
- **Blinking blue** – Bluetooth pairing/authentication mode.
