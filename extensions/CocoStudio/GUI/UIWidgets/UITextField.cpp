/****************************************************************************
 Copyright (c) 2013 cocos2d-x.org
 
 http://www.cocos2d-x.org
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#include "UITextField.h"

NS_CC_BEGIN

namespace gui {

UICCTextField::UICCTextField()
: _maxLengthEnabled(false)
, _maxLength(0)
, _passwordEnabled(false)
, _passwordStyleText("*")
, _attachWithIME(false)
, _detachWithIME(false)
, _insertText(false)
, _deleteBackward(false)
{
}

UICCTextField::~UICCTextField()
{
}

UICCTextField * UICCTextField::create(const char *placeholder, const char *fontName, float fontSize)
{
    UICCTextField *pRet = new UICCTextField();
    
    if(pRet && pRet->initWithString("", fontName, fontSize))
    {
        pRet->autorelease();
        if (placeholder)
        {
            pRet->setPlaceHolder(placeholder);
        }
        return pRet;
    }
    CC_SAFE_DELETE(pRet);
    
    return NULL;
}

void UICCTextField::onEnter()
{
    CCTextFieldTTF::setDelegate(this);
}


bool UICCTextField::onTextFieldAttachWithIME(CCTextFieldTTF *pSender)
{
    setAttachWithIME(true);
    return false;
}

bool UICCTextField::onTextFieldInsertText(CCTextFieldTTF *pSender, const char *text, int nLen)
{
    if (nLen == 1 && strcmp(text, "\n") == 0)
    {
        return false;
    }
    setInsertText(true);
    if (_maxLengthEnabled)
    {
        if (CCTextFieldTTF::getCharCount() >= _maxLength)
        {
            return true;
        }
    }
    
    return false;
}

bool UICCTextField::onTextFieldDeleteBackward(CCTextFieldTTF *pSender, const char *delText, int nLen)
{
    setDeleteBackward(true);
    return false;
}

bool UICCTextField::onTextFieldDetachWithIME(CCTextFieldTTF *pSender)
{
    setDetachWithIME(true);
    return false;
}

void UICCTextField::insertText(const char * text, int len)
{
    std::string str_text = text;
    int str_len = std::strlen(CCTextFieldTTF::getString());
    
    if (strcmp(text, "\n") != 0)
    {
        if (_maxLengthEnabled)
        {
            int multiple = 1;
            char value = text[0];
            if (value < 0 || value > 127)
            {
                multiple = 3;
            }
            
            if (str_len + len > _maxLength * multiple)
            {
                str_text = str_text.substr(0, _maxLength * multiple);
                len = _maxLength * multiple;
                /*
                 int mod = str_len % 3;
                 int offset = (mod == 0) ? 0 : (3 - mod);
                 int amount = str_len + offset;
                 str_text = str_text.substr(0, _maxLength - amount);
                 //                CCLOG("str_test = %s", str_text.c_str());
                 */
            }
        }
    }
    CCTextFieldTTF::insertText(str_text.c_str(), len);
    
    // password
    if (_passwordEnabled)
    {
        if (CCTextFieldTTF::getCharCount() > 0)
        {
            setPasswordText(m_pInputText->c_str());
        }
    }
}

void UICCTextField::deleteBackward()
{
    CCTextFieldTTF::deleteBackward();
    
    if (CCTextFieldTTF::getCharCount() > 0)
    {
        // password
        if (_passwordEnabled)
        {
            setPasswordText(m_pInputText->c_str());
        }
    }
}

void UICCTextField::openIME()
{
    CCTextFieldTTF::attachWithIME();
}

void UICCTextField::closeIME()
{
    CCTextFieldTTF::detachWithIME();
}

void UICCTextField::setMaxLengthEnabled(bool enable)
{
    _maxLengthEnabled = enable;
}

bool UICCTextField::isMaxLengthEnabled()
{
    return _maxLengthEnabled;
}

void UICCTextField::setMaxLength(int length)
{
    _maxLength = length;
}

int UICCTextField::getMaxLength()
{
    return _maxLength;
}

int UICCTextField::getCharCount()
{
    return CCTextFieldTTF::getCharCount();
}

void UICCTextField::setPasswordEnabled(bool enable)
{
    _passwordEnabled = enable;
}

bool UICCTextField::isPasswordEnabled()
{
    return _passwordEnabled;
}

void UICCTextField::setPasswordStyleText(const char* styleText)
{
    if (strlen(styleText) > 1)
    {
        return;
    }
    char value = styleText[0];
    if (value < 33 || value > 126)
    {
        return;
    }
    _passwordStyleText = styleText;
}

void UICCTextField::setPasswordText(const char *text)
{
    std::string tempStr;
    for (size_t i = 0; i < strlen(text); ++i)
    {
        tempStr.append(_passwordStyleText);
    }
    CCLabelTTF::setString(tempStr.c_str());
}

void UICCTextField::setAttachWithIME(bool attach)
{
    _attachWithIME = attach;
}

bool UICCTextField::getAttachWithIME()
{
    return _attachWithIME;
}

void UICCTextField::setDetachWithIME(bool detach)
{
    _detachWithIME = detach;
}

bool UICCTextField::getDetachWithIME()
{
    return _detachWithIME;
}

void UICCTextField::setInsertText(bool insert)
{
    _insertText = insert;
}

bool UICCTextField::getInsertText()
{
    return _insertText;
}

void UICCTextField::setDeleteBackward(bool deleteBackward)
{
    _deleteBackward = deleteBackward;
}

bool UICCTextField::getDeleteBackward()
{
    return _deleteBackward;
}


static const int TEXTFIELD_RENDERER_Z = (-1);
    
TextField::TextField():
_textFieldRenderer(NULL),
_touchWidth(0.0f),
_touchHeight(0.0f),
_useTouchArea(false),
_textFieldEventListener(NULL),
_textFieldEventSelector(NULL),
_passwordStyleText("")
{
}

TextField::~TextField()
{
    _textFieldEventListener = NULL;
    _textFieldEventSelector = NULL;
}

TextField* TextField::create()
{
    TextField* widget = new TextField();
    if (widget && widget->init())
    {
        widget->autorelease();
        return widget;
    }
    CC_SAFE_DELETE(widget);
    return NULL;
}

void TextField::onEnter()
{
    Widget::onEnter();
    scheduleUpdate();
}

void TextField::initRenderer()
{
    _textFieldRenderer = UICCTextField::create("input words here", "Thonburi", 20);
    CCNodeRGBA::addChild(_textFieldRenderer, TEXTFIELD_RENDERER_Z, -1);
}

void TextField::setTouchSize(const CCSize &size)
{
    _useTouchArea = true;
    _touchWidth = size.width;
    _touchHeight = size.height;
}
    
CCSize TextField::getTouchSize()
{
    return CCSizeMake(_touchWidth, _touchHeight);
}

void TextField::setText(const std::string& text)
{
    std::string strText(text);
    if (isMaxLengthEnabled())
    {
        strText = strText.substr(0, getMaxLength());
    }
    const char* content = strText.c_str();
    if (isPasswordEnabled())
    {
        _textFieldRenderer->setPasswordText(content);
        _textFieldRenderer->insertText(content, strlen(content));
    }
    else
    {
        _textFieldRenderer->setString(content);
    }
    textfieldRendererScaleChangedWithSize();
}

void TextField::setPlaceHolder(const std::string& value)
{
    _textFieldRenderer->setPlaceHolder(value.c_str());
    textfieldRendererScaleChangedWithSize();
}
    
const char* TextField::getPlaceHolder()
{
    return _textFieldRenderer->getPlaceHolder();
}

void TextField::setFontSize(int size)
{
    _textFieldRenderer->setFontSize(size);
    textfieldRendererScaleChangedWithSize();
}

int TextField::getFontSize()
{
    return _textFieldRenderer->getFontSize();
}

void TextField::setFontName(const std::string& name)
{
    _textFieldRenderer->setFontName(name.c_str());
    textfieldRendererScaleChangedWithSize();
}
    
const char* TextField::getFontName()
{
    return _textFieldRenderer->getFontName();
}

void TextField::didNotSelectSelf()
{
    _textFieldRenderer->detachWithIME();
}

const char* TextField::getStringValue()
{
    return _textFieldRenderer->getString();
}

bool TextField::onTouchBegan(CCTouch *touch, CCEvent *unused_event)
{
    bool pass = Widget::onTouchBegan(touch, unused_event);
    if (_hitted)
    {
        _textFieldRenderer->attachWithIME();
    }
    return pass;
}

void TextField::setMaxLengthEnabled(bool enable)
{
    _textFieldRenderer->setMaxLengthEnabled(enable);
}

bool TextField::isMaxLengthEnabled()
{
    return _textFieldRenderer->isMaxLengthEnabled();
}

void TextField::setMaxLength(int length)
{
    _textFieldRenderer->setMaxLength(length);
}

int TextField::getMaxLength()
{
    return _textFieldRenderer->getMaxLength();
}

void TextField::setPasswordEnabled(bool enable)
{
    _textFieldRenderer->setPasswordEnabled(enable);
}

bool TextField::isPasswordEnabled()
{
    return _textFieldRenderer->isPasswordEnabled();
}

void TextField::setPasswordStyleText(const char *styleText)
{
    _textFieldRenderer->setPasswordStyleText(styleText);
    
    _passwordStyleText = styleText;
}
    
const char* TextField::getPasswordStyleText()
{
    return _passwordStyleText.c_str();
}

void TextField::update(float dt)
{
    if (getAttachWithIME())
    {
        attachWithIMEEvent();
        setAttachWithIME(false);
    }
    if (getDetachWithIME())
    {
        detachWithIMEEvent();
        setDetachWithIME(false);
    }
    if (getInsertText())
    {
        insertTextEvent();
        setInsertText(false);
        
        textfieldRendererScaleChangedWithSize();
    }
    if (getDeleteBackward())
    {
        deleteBackwardEvent();
        setDeleteBackward(false);
    }
}

bool TextField::getAttachWithIME()
{
    return _textFieldRenderer->getAttachWithIME();
}

void TextField::setAttachWithIME(bool attach)
{
    _textFieldRenderer->setAttachWithIME(attach);
}

bool TextField::getDetachWithIME()
{
    return _textFieldRenderer->getDetachWithIME();
}

void TextField::setDetachWithIME(bool detach)
{
    _textFieldRenderer->setDetachWithIME(detach);
}

bool TextField::getInsertText()
{
    return _textFieldRenderer->getInsertText();
}

void TextField::setInsertText(bool insertText)
{
    _textFieldRenderer->setInsertText(insertText);
}

bool TextField::getDeleteBackward()
{
    return _textFieldRenderer->getDeleteBackward();
}

void TextField::setDeleteBackward(bool deleteBackward)
{
    _textFieldRenderer->setDeleteBackward(deleteBackward);
}

void TextField::attachWithIMEEvent()
{
    if (_textFieldEventListener && _textFieldEventSelector)
    {
        (_textFieldEventListener->*_textFieldEventSelector)(this, TEXTFIELD_EVENT_ATTACH_WITH_IME);
    }
}

void TextField::detachWithIMEEvent()
{
    if (_textFieldEventListener && _textFieldEventSelector)
    {
        (_textFieldEventListener->*_textFieldEventSelector)(this, TEXTFIELD_EVENT_DETACH_WITH_IME);
    }
}

void TextField::insertTextEvent()
{
    if (_textFieldEventListener && _textFieldEventSelector)
    {
        (_textFieldEventListener->*_textFieldEventSelector)(this, TEXTFIELD_EVENT_INSERT_TEXT);
    }
}

void TextField::deleteBackwardEvent()
{
    if (_textFieldEventListener && _textFieldEventSelector)
    {
        (_textFieldEventListener->*_textFieldEventSelector)(this, TEXTFIELD_EVENT_DELETE_BACKWARD);
    }
}

void TextField::addEventListenerTextField(CCObject *target, SEL_TextFieldEvent selecor)
{
    _textFieldEventListener = target;
    _textFieldEventSelector = selecor;
}

void TextField::setAnchorPoint(const CCPoint &pt)
{
    Widget::setAnchorPoint(pt);
    _textFieldRenderer->setAnchorPoint(pt);
}

void TextField::onSizeChanged()
{
    Widget::onSizeChanged();
    textfieldRendererScaleChangedWithSize();
}

void TextField::textfieldRendererScaleChangedWithSize()
{
    if (_ignoreSize)
    {
        _textFieldRenderer->setScale(1.0f);
        _size = getContentSize();
    }
    else
    {
        CCSize textureSize = getContentSize();
        if (textureSize.width <= 0.0f || textureSize.height <= 0.0f)
        {
            _textFieldRenderer->setScale(1.0f);
            return;
        }
        float scaleX = _size.width / textureSize.width;
        float scaleY = _size.height / textureSize.height;
        _textFieldRenderer->setScaleX(scaleX);
        _textFieldRenderer->setScaleY(scaleY);
    }
}

const CCSize& TextField::getContentSize() const
{
    return _textFieldRenderer->getContentSize();
}

CCNode* TextField::getVirtualRenderer()
{
    return _textFieldRenderer;
}

std::string TextField::getDescription() const
{
    return "TextField";
}

void TextField::attachWithIME()
{
    _textFieldRenderer->attachWithIME();
}

Widget* TextField::createCloneInstance()
{
    return TextField::create();
}

void TextField::copySpecialProperties(Widget *widget)
{
    TextField* textField = dynamic_cast<TextField*>(widget);
    if (textField)
    {
        setText(textField->_textFieldRenderer->getString());
        setPlaceHolder(textField->getStringValue());
        setFontSize(textField->_textFieldRenderer->getFontSize());
        setFontName(textField->_textFieldRenderer->getFontName());
        setMaxLengthEnabled(textField->isMaxLengthEnabled());
        setMaxLength(textField->getMaxLength());
        setPasswordEnabled(textField->isPasswordEnabled());
        setPasswordStyleText(textField->_passwordStyleText.c_str());
        setAttachWithIME(textField->getAttachWithIME());
        setDetachWithIME(textField->getDetachWithIME());
        setInsertText(textField->getInsertText());
        setDeleteBackward(textField->getDeleteBackward());
    }
}

}

NS_CC_END