function Component()
{
    // default constructor
}

Component.prototype.createOperations = function()
{
    try {
        // call the base create operations function
        component.createOperations();

        if (systemInfo.productType === "windows") {
            try {
                var userProfile = installer.environmentVariable("USERPROFILE");
                installer.setValue("UserProfile", userProfile);
                component.addOperation("CreateShortcut",
                    "@TargetDir@/chat.exe",
                    "@UserProfile@/Desktop/chat.lnk",
                    "workingDirectory=@TargetDir@",
                    "iconPath=%SystemRoot%/system32/SHELL32.dll",
                    "iconId=2", "description=Open GPT4All Chat");
            } catch (e) {
                print("ERROR: creating desktop shortcut" + e);
            }
            component.addOperation("CreateShortcut",
                "@TargetDir@/chat.exe",
                "@StartMenuDir@/chat.lnk",
                "workingDirectory=@TargetDir@",
                "iconPath=%SystemRoot%/system32/SHELL32.dll",
                "iconId=2", "description=Open GPT4All Chat");
        } else if (systemInfo.productType === "osx") {
        } else { // linux
            component.addOperation("CreateDesktopEntry", "/usr/share/applications/GPT4AllChat.desktop", "Version=0.1.0\nType=Application\nTerminal=false\nExec=@TargetDir@/chat\nName=GPT4All-Chat\nIcon=@TargetDir@YourApp_icon.png\nName[en_US]=GPT4All-Chat");
            component.addElevatedOperation("Copy", "/usr/share/applications/GPT4AllChat.desktop", "@HomeDir@/Desktop/GPT4AllChat.desktop");
        }
    } catch (e) {
        print("ERROR: creating desktop/startmenu shortcuts" + e);
    }
}