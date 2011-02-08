//
// Urho3D Engine
// Copyright (c) 2008-2011 Lasse ��rni
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "Precompiled.h"
#include "Button.h"
#include "CheckBox.h"
#include "Cursor.h"
#include "Engine.h"
#include "Font.h"
#include "LineEdit.h"
#include "MenuItem.h"
#include "RegisterTemplates.h"
#include "ScrollView.h"
#include "Slider.h"
#include "Text.h"
#include "UI.h"
#include "Window.h"

static void registerFont(asIScriptEngine* engine)
{
    registerResource<Font>(engine, "Font");
    registerRefCasts<Resource, Font>(engine, "Resource", "Font");
}

static void registerUIElement(asIScriptEngine* engine)
{
    engine->RegisterEnum("HorizontalAlignment");
    engine->RegisterEnumValue("HorizontalAlignment", "HA_LEFT", HA_LEFT);
    engine->RegisterEnumValue("HorizontalAlignment", "HA_CENTER", HA_CENTER);
    engine->RegisterEnumValue("HorizontalAlignment", "HA_RIGHT", HA_RIGHT);
    
    engine->RegisterEnum("VerticalAlignment");
    engine->RegisterEnumValue("VerticalAlignment", "VA_TOP", VA_TOP);
    engine->RegisterEnumValue("VerticalAlignment", "VA_CENTER", VA_CENTER);
    engine->RegisterEnumValue("VerticalAlignment", "VA_BOTTOM", VA_BOTTOM);
    
    engine->RegisterEnum("UIElementCorner");
    engine->RegisterEnumValue("UIElementCorner", "C_TOPLEFT", C_TOPLEFT);
    engine->RegisterEnumValue("UIElementCorner", "C_TOPRIGHT", C_TOPRIGHT);
    engine->RegisterEnumValue("UIElementCorner", "C_BOTTOMLEFT", C_BOTTOMLEFT);
    engine->RegisterEnumValue("UIElementCorner", "C_BOTTOMRIGHT", C_BOTTOMRIGHT);
    
    engine->RegisterEnum("UIElementOrientation");
    engine->RegisterEnumValue("UIElementOrientation", "O_HORIZONTAL", O_HORIZONTAL);
    engine->RegisterEnumValue("UIElementOrientation", "O_VERTICAL", O_VERTICAL);
    
    registerUIElement<UIElement>(engine, "UIElement");
    
    // Register static function for getting UI style XML element
    engine->RegisterGlobalFunction("XMLElement getStyleElement(XMLFile@+, const string& in)", asFUNCTIONPR(UIElement::getStyleElement, (XMLFile*, const std::string&), XMLElement), asCALL_CDECL);
    
    // Register Variant getPtr() for UIElement
    engine->RegisterObjectMethod("Variant", "UIElement@+ getUIElement() const", asFUNCTION(getVariantPtr<UIElement>), asCALL_CDECL_OBJLAST);
}

static void registerBorderImage(asIScriptEngine* engine)
{
    registerBorderImage<BorderImage>(engine, "BorderImage");
    registerRefCasts<UIElement, BorderImage>(engine, "UIElement", "BorderImage");
}

static void registerCursor(asIScriptEngine* engine)
{
    registerBorderImage<Cursor>(engine, "Cursor");
    engine->RegisterObjectMethod("Cursor", "void setHotspot(const IntVector2& in)", asMETHODPR(Cursor, setHotspot, (const IntVector2&), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("Cursor", "void setHotspot(int, int)", asMETHODPR(Cursor, setHotspot, (int, int), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("Cursor", "const IntVector2& getHotspot() const", asMETHOD(Cursor, getHotspot), asCALL_THISCALL);
    registerRefCasts<UIElement, Cursor>(engine, "UIElement", "Cursor");
}

static void registerButton(asIScriptEngine* engine)
{
    registerButton<Button>(engine, "Button");
    registerRefCasts<UIElement, Button>(engine, "UIElement", "Button");
}

static void registerCheckBox(asIScriptEngine* engine)
{
    registerBorderImage<CheckBox>(engine, "CheckBox");
    engine->RegisterObjectMethod("CheckBox", "void setChecked(bool)", asMETHOD(CheckBox, setChecked), asCALL_THISCALL);
    engine->RegisterObjectMethod("CheckBox", "void setUncheckedRect(const IntRect& in)", asMETHODPR(CheckBox, setUncheckedRect, (const IntRect&), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("CheckBox", "void setUncheckedRect(int, int, int, int)", asMETHODPR(CheckBox, setUncheckedRect, (int, int, int, int), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("CheckBox", "void setCheckedRect(const IntRect& in)", asMETHODPR(CheckBox, setCheckedRect, (const IntRect&), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("CheckBox", "void setCheckedRect(int, int, int, int)", asMETHODPR(CheckBox, setCheckedRect, (int, int, int, int), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("CheckBox", "bool isChecked() const", asMETHOD(CheckBox, isChecked), asCALL_THISCALL);
    engine->RegisterObjectMethod("CheckBox", "const IntRect& getUncheckedRect() const", asMETHOD(CheckBox, getUncheckedRect), asCALL_THISCALL);
    engine->RegisterObjectMethod("CheckBox", "const IntRect& getCheckedRect() const", asMETHOD(CheckBox, getCheckedRect), asCALL_THISCALL);
    registerRefCasts<UIElement, CheckBox>(engine, "UIElement", "CheckBox");
}

static void registerSlider(asIScriptEngine* engine)
{
    registerBorderImage<Slider>(engine, "Slider");
    engine->RegisterObjectMethod("Slider", "void setOrientation(UIElementOrientation)", asMETHOD(Slider, setOrientation), asCALL_THISCALL);
    engine->RegisterObjectMethod("Slider", "void setRange(float)", asMETHOD(Slider, setRange), asCALL_THISCALL);
    engine->RegisterObjectMethod("Slider", "void setValue(float)", asMETHOD(Slider, setValue), asCALL_THISCALL);
    engine->RegisterObjectMethod("Slider", "UIElementOrientation getOrientation() const", asMETHOD(Slider, getOrientation), asCALL_THISCALL);
    engine->RegisterObjectMethod("Slider", "float getRange() const", asMETHOD(Slider, getRange), asCALL_THISCALL);
    engine->RegisterObjectMethod("Slider", "float getValue() const", asMETHOD(Slider, getValue), asCALL_THISCALL);
    engine->RegisterObjectMethod("Slider", "BorderImage@+ getSliderElement() const", asMETHOD(Slider, getSliderElement), asCALL_THISCALL);
    registerRefCasts<UIElement, Slider>(engine, "UIElement", "Slider");
}

static void registerScrollView(asIScriptEngine* engine)
{
    registerBorderImage<ScrollView>(engine, "ScrollView");
    engine->RegisterObjectMethod("ScrollView", "void setViewPosition(const IntVector2& in)", asMETHODPR(ScrollView, setViewPosition, (const IntVector2&), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("ScrollView", "void setViewPosition(int, int)", asMETHODPR(ScrollView, setViewPosition, (int, int), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("ScrollView", "void setViewSize(const IntVector2& in)", asMETHODPR(ScrollView, setViewSize, (const IntVector2&), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("ScrollView", "void setViewSize(int, int)", asMETHODPR(ScrollView, setViewSize, (int, int), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("ScrollView", "void setHorizontalSlider(Slider@+)", asMETHOD(ScrollView, setHorizontalSlider), asCALL_THISCALL);
    engine->RegisterObjectMethod("ScrollView", "void setVerticalSlider(Slider@+)", asMETHOD(ScrollView, setVerticalSlider), asCALL_THISCALL);
    engine->RegisterObjectMethod("ScrollView", "const IntVector2& getViewPosition() const", asMETHOD(ScrollView, getViewPosition), asCALL_THISCALL);
    engine->RegisterObjectMethod("ScrollView", "const IntVector2& getViewSize() const", asMETHOD(ScrollView, getViewSize), asCALL_THISCALL);
    engine->RegisterObjectMethod("ScrollView", "Slider@+ getHorizontalSlider() const", asMETHOD(ScrollView, getHorizontalSlider), asCALL_THISCALL);
    engine->RegisterObjectMethod("ScrollView", "Slider@+ getVerticalSlider() const", asMETHOD(ScrollView, getVerticalSlider), asCALL_THISCALL);
    registerRefCasts<UIElement, ScrollView>(engine, "UIElement", "ScrollView");
}

static void registerText(asIScriptEngine* engine)
{
    registerUIElement<Text>(engine, "Text");
    engine->RegisterObjectMethod("Text", "bool setFont(Font@+, int)", asMETHOD(Text, setFont), asCALL_THISCALL);
    engine->RegisterObjectMethod("Text", "void setMaxWidth(int)", asMETHOD(Text, setMaxWidth), asCALL_THISCALL);
    engine->RegisterObjectMethod("Text", "void setText(const string& in)", asMETHOD(Text, setText), asCALL_THISCALL);
    engine->RegisterObjectMethod("Text", "void setTextAlignment(HorizontalAlignment)", asMETHOD(Text, setTextAlignment), asCALL_THISCALL);
    engine->RegisterObjectMethod("Text", "void setRowSpacing(float)", asMETHOD(Text, setRowSpacing), asCALL_THISCALL);
    engine->RegisterObjectMethod("Text", "void setSelection(uint, uint)", asMETHOD(Text, setSelection), asCALL_THISCALL);
    engine->RegisterObjectMethod("Text", "void clearSelection()", asMETHOD(Text, clearSelection), asCALL_THISCALL);
    engine->RegisterObjectMethod("Text", "void setSelectionColor(const Color& in)", asMETHOD(Text, setSelectionColor), asCALL_THISCALL);
    engine->RegisterObjectMethod("Text", "void setHoverColor(const Color& in)", asMETHOD(Text, setHoverColor), asCALL_THISCALL);
    engine->RegisterObjectMethod("Text", "Font@+ getFont() const", asMETHOD(Text, getFont), asCALL_THISCALL);
    engine->RegisterObjectMethod("Text", "int getFontSize() const", asMETHOD(Text, getFontSize), asCALL_THISCALL);
    engine->RegisterObjectMethod("Text", "int getMaxWidth() const", asMETHOD(Text, getMaxWidth), asCALL_THISCALL);
    engine->RegisterObjectMethod("Text", "const string& getText() const", asMETHOD(Text, getText), asCALL_THISCALL);
    engine->RegisterObjectMethod("Text", "HorizontalAlignment getTextAlignment() const", asMETHOD(Text, getTextAlignment), asCALL_THISCALL);
    engine->RegisterObjectMethod("Text", "float getRowSpacing() const", asMETHOD(Text, getRowSpacing), asCALL_THISCALL);
    engine->RegisterObjectMethod("Text", "uint getSelectionStart() const", asMETHOD(Text, getSelectionStart), asCALL_THISCALL);
    engine->RegisterObjectMethod("Text", "uint getSelectionLength() const", asMETHOD(Text, getSelectionLength), asCALL_THISCALL);
    engine->RegisterObjectMethod("Text", "const Color& getSelectionColor() const", asMETHOD(Text, getSelectionColor), asCALL_THISCALL);
    engine->RegisterObjectMethod("Text", "const Color& getHoverColor() const", asMETHOD(Text, getHoverColor), asCALL_THISCALL);
    engine->RegisterObjectMethod("Text", "uint getNumRows() const", asMETHOD(Text, getNumRows), asCALL_THISCALL);
    engine->RegisterObjectMethod("Text", "int getRowHeight() const", asMETHOD(Text, getRowHeight), asCALL_THISCALL);
    registerRefCasts<UIElement, Text>(engine, "UIElement", "Text");
}

static void registerLineEdit(asIScriptEngine* engine)
{
    registerBorderImage<LineEdit>(engine, "LineEdit");
    engine->RegisterObjectMethod("LineEdit", "void setText(const string& in)", asMETHOD(LineEdit, setText), asCALL_THISCALL);
    engine->RegisterObjectMethod("LineEdit", "void setCursorPosition(uint)", asMETHOD(LineEdit, setCursorPosition), asCALL_THISCALL);
    engine->RegisterObjectMethod("LineEdit", "void setCursorBlinkRate(float)", asMETHOD(LineEdit, setCursorBlinkRate), asCALL_THISCALL);
    engine->RegisterObjectMethod("LineEdit", "void setMaxLength(uint)", asMETHOD(LineEdit, setMaxLength), asCALL_THISCALL);
    engine->RegisterObjectMethod("LineEdit", "void setEchoCharacter(uint8)", asMETHOD(LineEdit, setEchoCharacter), asCALL_THISCALL);
    engine->RegisterObjectMethod("LineEdit", "void setDefocusable(bool)", asMETHOD(LineEdit, setDefocusable), asCALL_THISCALL);
    engine->RegisterObjectMethod("LineEdit", "void setCursorMovable(bool)", asMETHOD(LineEdit, setCursorMovable), asCALL_THISCALL);
    engine->RegisterObjectMethod("LineEdit", "void setTextSelectable(bool)", asMETHOD(LineEdit, setTextSelectable), asCALL_THISCALL);
    engine->RegisterObjectMethod("LineEdit", "void setTextCopyable(bool)", asMETHOD(LineEdit, setTextCopyable), asCALL_THISCALL);
    engine->RegisterObjectMethod("LineEdit", "const string& getText() const", asMETHOD(LineEdit, getText), asCALL_THISCALL);
    engine->RegisterObjectMethod("LineEdit", "uint getCursorPosition() const", asMETHOD(LineEdit, getCursorPosition), asCALL_THISCALL);
    engine->RegisterObjectMethod("LineEdit", "float getCursorBlinkRate() const", asMETHOD(LineEdit, getCursorBlinkRate), asCALL_THISCALL);
    engine->RegisterObjectMethod("LineEdit", "uint getMaxLength() const", asMETHOD(LineEdit, getMaxLength), asCALL_THISCALL);
    engine->RegisterObjectMethod("LineEdit", "uint8 getEchoCharacter() const", asMETHOD(LineEdit, getEchoCharacter), asCALL_THISCALL);
    engine->RegisterObjectMethod("LineEdit", "bool isDefocusable() const", asMETHOD(LineEdit, isDefocusable), asCALL_THISCALL);
    engine->RegisterObjectMethod("LineEdit", "bool isCursorMovable() const", asMETHOD(LineEdit, isCursorMovable), asCALL_THISCALL);
    engine->RegisterObjectMethod("LineEdit", "bool isTextSelectable() const", asMETHOD(LineEdit, isTextSelectable), asCALL_THISCALL);
    engine->RegisterObjectMethod("LineEdit", "bool isTextCopyable() const", asMETHOD(LineEdit, isTextCopyable), asCALL_THISCALL);
    engine->RegisterObjectMethod("LineEdit", "Text@+ getTextElement() const", asMETHOD(LineEdit, getTextElement), asCALL_THISCALL);
    engine->RegisterObjectMethod("LineEdit", "BorderImage@+ getCursorElement() const", asMETHOD(LineEdit, getCursorElement), asCALL_THISCALL);
    registerRefCasts<UIElement, LineEdit>(engine, "UIElement", "LineEdit");
}

static void registerMenuItem(asIScriptEngine* engine)
{
    registerButton<MenuItem>(engine, "MenuItem");
    engine->RegisterObjectMethod("MenuItem", "void setPopup(UIElement@+)", asMETHOD(MenuItem, setPopup), asCALL_THISCALL);
    engine->RegisterObjectMethod("MenuItem", "void setPopupOffset(const IntVector2& in)", asMETHODPR(MenuItem, setPopupOffset, (const IntVector2&), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("MenuItem", "void setPopupOffset(int, int)", asMETHODPR(MenuItem, setPopupOffset, (int, int), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("MenuItem", "void showPopup(bool)", asMETHOD(MenuItem, showPopup), asCALL_THISCALL);
    engine->RegisterObjectMethod("MenuItem", "UIElement@+ getPopup() const", asMETHOD(MenuItem, getPopup), asCALL_THISCALL);
    engine->RegisterObjectMethod("MenuItem", "const IntVector2& getPopupOffset() const", asMETHOD(MenuItem, getPopupOffset), asCALL_THISCALL);
    engine->RegisterObjectMethod("MenuItem", "bool getShowPopup() const", asMETHOD(MenuItem, getShowPopup), asCALL_THISCALL);
    registerRefCasts<UIElement, MenuItem>(engine, "UIElement", "MenuItem");
}

static void registerWindow(asIScriptEngine* engine)
{
    registerBorderImage<Window>(engine, "Window");
    engine->RegisterObjectMethod("Window", "void setMovable(bool)", asMETHOD(Window, setMovable), asCALL_THISCALL);
    engine->RegisterObjectMethod("Window", "void setResizable(bool)", asMETHOD(Window, setResizable), asCALL_THISCALL);
    engine->RegisterObjectMethod("Window", "void setMinSize(const IntVector2& in)", asMETHODPR(Window, setMinSize, (const IntVector2&), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("Window", "void setMinSize(int, int)", asMETHODPR(Window, setMinSize, (int, int), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("Window", "void setMaxSize(const IntVector2& in)", asMETHODPR(Window, setMaxSize, (const IntVector2&), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("Window", "void setMaxSize(int, int)", asMETHODPR(Window, setMaxSize, (int, int), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("Window", "void setResizeBorder(const IntRect& in)", asMETHODPR(Window, setResizeBorder, (const IntRect&), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("Window", "void setResizeBorder(int, int, int, int)", asMETHODPR(Window, setResizeBorder, (int, int, int, int), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("Window", "bool isMovable() const", asMETHOD(Window, isMovable), asCALL_THISCALL);
    engine->RegisterObjectMethod("Window", "bool isResizable() const", asMETHOD(Window, isResizable), asCALL_THISCALL);
    engine->RegisterObjectMethod("Window", "const IntVector2& getMinSize() const", asMETHOD(Window, getMinSize), asCALL_THISCALL);
    engine->RegisterObjectMethod("Window", "const IntVector2& getMaxSize() const", asMETHOD(Window, getMaxSize), asCALL_THISCALL);
    engine->RegisterObjectMethod("Window", "const IntRect& getResizeBorder() const", asMETHOD(Window, getResizeBorder), asCALL_THISCALL);
    registerRefCasts<UIElement, Window>(engine, "UIElement", "Window");
}

static UI* GetUI()
{
    return getEngine()->getUI();
}

static UIElement* GetUIRoot()
{
    return getEngine()->getUIRoot();
}

static Cursor* GetUICursor()
{
    return getEngine()->getUICursor();
}

static UIElement* UICreateElement(const std::string& type, const std::string& name, UI* ptr)
{
    try
    {
        SharedPtr<UIElement> element = ptr->createElement(ShortStringHash(type), name);
        // The shared pointer will go out of scope, so have to increment the reference count
        // (here an auto handle can not be used)
        if (element)
            element->addRef();
        return element.getPtr();
    }
    catch (Exception& e)
    {
        SAFE_RETHROW_RET(e, 0);
    }
}

static UIElement* UILoadLayout(XMLFile* file, UI* ptr)
{
    try
    {
        SharedPtr<UIElement> root = ptr->loadLayout(file);
        if (root)
            root->addRef();
        return root.getPtr();
    }
    catch (Exception& e)
    {
        SAFE_RETHROW_RET(e, 0);
    }
}

static UIElement* UILoadLayoutWithStyle(XMLFile* file, XMLFile* styleFile, UI* ptr)
{
    try
    {
        SharedPtr<UIElement> root = ptr->loadLayout(file, styleFile);
        if (root)
            root->addRef();
        return root.getPtr();
    }
    catch (Exception& e)
    {
        SAFE_RETHROW_RET(e, 0);
    }
}

static void registerUI(asIScriptEngine* engine)
{
    engine->RegisterObjectType("UI", 0, asOBJ_REF);
    engine->RegisterObjectBehaviour("UI", asBEHAVE_ADDREF, "void f()", asMETHOD(UI, addRef), asCALL_THISCALL);
    engine->RegisterObjectBehaviour("UI", asBEHAVE_RELEASE, "void f()", asMETHOD(UI, releaseRef), asCALL_THISCALL);
    engine->RegisterObjectMethod("UI", "void setCursor(Cursor@+)", asMETHOD(UI, setCursor), asCALL_THISCALL);
    engine->RegisterObjectMethod("UI", "void setFocusElement(UIElement@+)", asMETHOD(UI, setFocusElement), asCALL_THISCALL);
    engine->RegisterObjectMethod("UI", "void clear()", asMETHOD(UI, clear), asCALL_THISCALL);
    engine->RegisterObjectMethod("UI", "UIElement@ createElement(const string& in, const string& in)", asFUNCTION(UICreateElement), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("UI", "UIElement@ loadLayout(XMLFile@+)", asFUNCTION(UILoadLayout), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("UI", "UIElement@ loadLayout(XMLFile@+, XMLFile@+)", asFUNCTION(UILoadLayoutWithStyle), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("UI", "UIElement@+ getRootElement() const", asMETHOD(UI, getRootElement), asCALL_THISCALL);
    engine->RegisterObjectMethod("UI", "Cursor@+ getCursor() const", asMETHOD(UI, getCursor), asCALL_THISCALL);
    engine->RegisterObjectMethod("UI", "UIElement@+ getElementAt(const IntVector2& in, bool)", asMETHODPR(UI, getElementAt, (const IntVector2&, bool), UIElement*), asCALL_THISCALL);
    engine->RegisterObjectMethod("UI", "UIElement@+ getElementAt(int, int, bool)", asMETHODPR(UI, getElementAt, (int, int, bool), UIElement*), asCALL_THISCALL);
    engine->RegisterObjectMethod("UI", "UIElement@+ getFocusElement()", asMETHOD(UI, getFocusElement), asCALL_THISCALL);
    engine->RegisterObjectMethod("UI", "IntVector2 getCursorPosition()", asMETHOD(UI, getCursorPosition), asCALL_THISCALL);
    
    engine->RegisterGlobalFunction("UI@+ getUI()", asFUNCTION(GetUI), asCALL_CDECL);
    engine->RegisterGlobalFunction("UI@+ get_ui()", asFUNCTION(GetUI), asCALL_CDECL);
    engine->RegisterGlobalFunction("UIElement@+ getUIRoot()", asFUNCTION(GetUIRoot), asCALL_CDECL);
    engine->RegisterGlobalFunction("UIElement@+ get_uiRoot()", asFUNCTION(GetUIRoot), asCALL_CDECL);
    engine->RegisterGlobalFunction("Cursor@+ getUICursor()", asFUNCTION(GetUICursor), asCALL_CDECL);
    engine->RegisterGlobalFunction("Cursor@+ get_uiCursor()", asFUNCTION(GetUICursor), asCALL_CDECL);
}

void registerUILibrary(asIScriptEngine* engine)
{
    registerFont(engine);
    registerUIElement(engine);
    registerBorderImage(engine);
    registerButton(engine);
    registerCheckBox(engine);
    registerCursor(engine);
    registerSlider(engine);
    registerScrollView(engine);
    registerText(engine);
    registerLineEdit(engine);
    registerMenuItem(engine);
    registerWindow(engine);
    registerUI(engine);
}
