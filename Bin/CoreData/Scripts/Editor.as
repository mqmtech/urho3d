// Urho3D editor

#include "Scripts/EditorCamera.as"
#include "Scripts/EditorScene.as"
#include "Scripts/EditorUI.as"

void start()
{
    if (engine.isHeadless())
    {
        errorDialog("Urho3D Editor", "Headless mode is not supported. The program will now exit.");
        engine.exit();
        return;
    }
    
    // Free the mouse cursor
    input.setClipCursor(false);

    subscribeToEvent("Update", "handleUpdate");

    createScene();
    createCamera();
    createUI();
    updateWindowTitle();
    parseArguments();
}

void parseArguments()
{
    array<string> arguments = getArguments();

    // The first argument should be the editor script name. Scan for a scene to load
    for (uint i = 1; i < arguments.size(); ++i)
    {
        if (arguments[i][0] != '-')
        {
            loadScene(getAbsoluteFileName(arguments[i]));
            break;
        }
    }
}

void handleUpdate(StringHash eventType, VariantMap& eventData)
{
    float timeStep = eventData["TimeStep"].getFloat();

    moveCamera(timeStep);
}