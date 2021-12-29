inline bool
MatchUIID(ui_id ID1, ui_id ID2) {
    return ID1.Primary == ID2.Primary && ID1.Secondary == ID2.Secondary;
}

inline ui_id
UIIDNull() {
    return {0, 0};
}

internal ui_id
UI_GenerateID(ui *UI, char *Text) {
    ui_id ID = {};
    ID.Primary = HashCString(Text);
    
    uint32 SecondaryIndex = ID.Primary % UI_MAX_SECONDARY_IDENTIFIERS;
    ID.Secondary = UI->SecondaryIdentifier[SecondaryIndex]++;
    
    UI->UsedID[UI->UsedIDCount++] = ID;
    
    return ID;
}

internal ui_window *
UI_GetWindowByID(ui *UI, ui_id ID) {
    for (int WindowIndex = 0; WindowIndex < UI_MAX_WINDOWS; ++WindowIndex) {
        if (MatchUIID(UI->Windows[WindowIndex].ID, ID)) {
            return UI->Windows + WindowIndex;
        }
    }
    
    return 0;
}

internal char *
UI_StoreString(ui *UI, char *Text) {
    int Length = strlen(Text);
    
    char *Result = UI->StringBuffer + UI->StringBufferUsed;
    strcpy(Result, Text);
    UI->StringBufferUsed += Length + 1;
    
    return Result;
}

internal void
UI_SetupFrame(ui *UI) {
    bool FoundHot = false;
    bool FoundActive = false;
    
    for (int IDIndex = 0; IDIndex < UI->UsedIDCount; ++IDIndex) {
        ui_id ID = UI->UsedID[IDIndex];
        if (MatchUIID(ID, UI->Hot)) {
            FoundHot = true;
        }
        
        if (MatchUIID(ID, UI->Active)) {
            FoundActive = true;
        }
    }
    
    if (!FoundHot) {
        UI->Hot = UIIDNull();
    }
    
    if (!FoundActive) {
        UI->Active = UIIDNull();
    }
    
    UI->WidgetCount = 0;
    memset(UI->SecondaryIdentifier, 0, sizeof(UI->SecondaryIdentifier));
    
    UI->UsedIDCount = 0;
    
    UI->Position = V2(0, 0);
    UI->RowMode = true;
    
    UI->Width = 0;
    UI->Height = 0;
    
    UI->AutoWidth = true;
    UI->AutoHeight = true;
    
    UI->PositionStackCount = 0;
    UI->RowModeStackCount = 0;
    
    UI->WindowCount = 0;
    UI->ActiveWindow = 0;
    
    UI->StringBufferUsed = 0;
}

internal void
UI_RenderButtonBase(ui_widget *Widget, renderer *Renderer) {
    real32 BaseColor = 0.2f + Widget->HotTransition * 0.5f - Widget->ActiveTransition * 0.3f;
    real32 Alpha = 0.3f;
    
    v4 Color = V4(BaseColor, BaseColor, BaseColor, Alpha);
    
    Renderer_PushRect(Renderer, Widget->Position, Widget->Size, Color, 0);
    Renderer_PushHollowRect(Renderer, Widget->Position, Widget->Size, V4(1, 1, 1, 1), 0);
}

internal int
UI_DefaultRenderWidget(ui *UI, ui_widget *Widget, renderer *Renderer) {
    int Result = 0;
    
    Widget->HotTransition += ((real32)MatchUIID(Widget->ID, UI->Hot) - Widget->HotTransition) * 0.3f;
    Widget->ActiveTransition += ((real32)MatchUIID(Widget->ID, UI->Active) - Widget->ActiveTransition) * 0.3f;
    
    ui_style *Style = &UI->Style;
    
    switch (Widget->Type) {
        case UIWidget_button: {
            UI_RenderButtonBase(Widget, Renderer);
            v2 OffsettedPosition = Widget->Position + V2(UI->Style.Margin, UI->Style.Margin);
            
            Renderer_PushText(Renderer, Widget->Text, Style->Font, OffsettedPosition, V4(1, 1, 1, 1), 0);
        } break;
        
        case UIWidget_label: {
            Renderer_PushText(Renderer, Widget->Text, Style->Font, Widget->Position, V4(1, 1, 1, 1), 0);
        } break;
        
        case UIWidget_dropdown: {
            UI_RenderButtonBase(Widget, Renderer);
            v2 OffsettedPosition = Widget->Position + V2(UI->Style.Margin, UI->Style.Margin);
            
            Renderer_PushText(Renderer, Widget->Text, Style->Font, OffsettedPosition, V4(1, 1, 1, 1), 0);
            
            if (Widget->Dropdown.OpenTransition > 0.02f) {
                v2 DropdownPosition;
                
                if (Widget->Dropdown.BelowButton) {
                    DropdownPosition = V2(Widget->Position.X, Widget->Position.Y + Widget->Size.Height);
                } else {
                    DropdownPosition = V2(Widget->Position.X + Widget->Size.Width, Widget->Position.Y);
                }
                v2 DropdownSize = V2(200, Widget->Dropdown.FullHeight * Widget->Dropdown.OpenTransition);
                
                Renderer_PushClipRect(Renderer, V4(0, 0, Renderer->RenderWidth, Renderer->RenderHeight));
                Renderer_PushBlurRectangle(Renderer, DropdownPosition, DropdownSize);
                Renderer_PushRect(Renderer, DropdownPosition, DropdownSize, V4(0.2f, 0.2f, 0.2f, 0.3f), 0);
                Renderer_PushHollowRect(Renderer, DropdownPosition, DropdownSize, V4(1, 1, 1, 1), 0);
                
                Renderer_PushConstrainedClipRect(Renderer, V4(DropdownPosition, DropdownSize));
                
                for (int WidgetIndex = 0; WidgetIndex < Widget->Dropdown.WidgetCount; ++WidgetIndex) {
                    ui_widget *DropdownWidget = Widget + WidgetIndex + 1;
                    UI->Style.RenderWidget(UI, DropdownWidget, Renderer);
                }
                
                Renderer_PopClipRect(Renderer);
                Renderer_PopClipRect(Renderer);
            }
            
            // Skip the contained widgets.
            Result = Widget->Dropdown.WidgetCount;
        } break;
        
        case UIWidget_checkbox: {
            UI_RenderButtonBase(Widget, Renderer);
            v2 OffsettedPosition = Widget->Position + V2(UI->Style.Margin, UI->Style.Margin);
            
            Renderer_PushText(Renderer, Widget->Text, Style->Font, OffsettedPosition, V4(1, 1, 1, 1), 0);
            
            real32 CheckboxSize = Widget->Size.Y - UI->Style.Margin * 2;
            
            if (Widget->Toggled) {
                Renderer_PushRect(Renderer,
                                  Widget->Position + V2(Widget->Size.X - CheckboxSize - UI->Style.Margin,
                                                        UI->Style.Margin),
                                  V2(CheckboxSize),
                                  V4(0.3f, 0.5f, 0.9f, 0.5f),
                                  0);
            }
            
            Renderer_PushHollowRect(Renderer,
                                    Widget->Position + V2(Widget->Size.X - CheckboxSize - UI->Style.Margin,
                                                          UI->Style.Margin),
                                    V2(CheckboxSize),
                                    V4(1),
                                    0);
            
        } break;
        
        case UIWidget_slider: {
            real32 BaseColor = 0.2f + Widget->HotTransition * 0.5f - Widget->ActiveTransition * 0.3f;
            real32 Alpha = 0.3f;
            
            v4 Color = V4(BaseColor, BaseColor, BaseColor, Alpha);
            
            Renderer_PushRect(Renderer, Widget->Position, Widget->Size, Color, 0);
            Renderer_PushRect(Renderer,
                              Widget->Position,
                              V2(Widget->Size.X * Widget->SliderRelativeWidth, Widget->Size.Y),
                              V4(0.3f, 0.5f, 0.9f, 0.5f), 0);
            Renderer_PushHollowRect(Renderer, Widget->Position, Widget->Size, V4(1, 1, 1, 1), 0);
            
            v2 OffsettedPosition = Widget->Position + V2(UI->Style.Margin, UI->Style.Margin);
            
            Renderer_PushText(Renderer, Widget->Text, Style->Font, OffsettedPosition, V4(1, 1, 1, 1), 0);
        } break;
        
        case UIWidget_textbox: {
            real32 BaseColor = 0.05f + Widget->HotTransition * 0.3f - Widget->ActiveTransition * 0.2f;
            real32 Alpha = 0.3f;
            
            v4 Color = V4(BaseColor, BaseColor, BaseColor, Alpha);
            
            Renderer_PushRect(Renderer, Widget->Position, Widget->Size, Color, 0);
            Renderer_PushHollowRect(Renderer, Widget->Position, Widget->Size, V4(1, 1, 1, 1), 0);
            
            v2 OffsettedPosition = Widget->Position + V2(UI->Style.Margin, UI->Style.Margin);
            
            if (Platform->KeyboardRedirect == &Widget->TextBuffer) {
                v2 MarkerPosition = OffsettedPosition + V2(GetTextWidth(Style->Font, Widget->TextBuffer.Base), 0);
                Renderer_PushLine(Renderer,
                                  MarkerPosition, MarkerPosition + V2(0, GetTextHeight(Style->Font)),
                                  V4(1), 0);
            }
            
            Renderer_PushText(Renderer, Widget->TextBuffer.Base, Style->Font, OffsettedPosition, V4(1, 1, 1, 1), 0);
            
        } break;
    }
    
    return Result;
}

internal void
UI_RenderUI(ui *UI, renderer *Renderer) {
    for (int WindowIndex = UI->WindowCount - 1; WindowIndex >= 0; --WindowIndex) {
        ui_window *Window = UI_GetWindowByID(UI, UI->WindowOrder[WindowIndex]);
        
        if (Window) {
            Renderer_PushBlurRectangle(Renderer, Window->Position, Window->Size);
            Renderer_PushRect(Renderer, Window->Position, Window->Size, V4(0.2f, 0.2f, 0.2f, 0.3f), 0);
            Renderer_PushHollowRect(Renderer, Window->Position, Window->Size, V4(1, 1, 1, 1), 0);
            
            Renderer_PushConstrainedClipRect(Renderer, V4(Window->Position, Window->Size));
            
            if (Window->Flags & UI_WINDOW_FLAG_TITLEBAR) {
                v2 Size = V2(Window->Size.X, Window->TitlebarHeight);
                
                Renderer_PushRect(Renderer, Window->Position, Size, V4(0.2f, 0.2f, 0.2f, 0.3f), 0);
                Renderer_PushHollowRect(Renderer, Window->Position, Size, V4(1, 1, 1, 1), 0);
            }
            
            for (int WidgetIndex = Window->StartWidgetIndex; WidgetIndex < Window->EndWidgetIndex; ++WidgetIndex) {
                ui_widget *Widget = UI->Widgets + WidgetIndex;
                
                // If a widget requests to skip the next widgets, that's done here.
                WidgetIndex += UI->Style.RenderWidget(UI, Widget, Renderer);
            }
            
            Renderer_PopClipRect(Renderer);
        }
    }
    
    for (int WidgetIndex = 0; WidgetIndex < UI->WidgetCount; ++WidgetIndex) {
        ui_widget *Widget = UI->Widgets + WidgetIndex;
        
        if (Widget->Window == 0) {
            // If a widget requests to skip the next widgets, that's done here.
            WidgetIndex += UI->Style.RenderWidget(UI, Widget, Renderer);
        }
    }
}

internal bool
UI_TestForOverlappingWindows(ui *UI, ui_window *Window, v2 Position) {
    int CurrentWindowIndex = -1;
    
    for (int WindowIndex = 0; WindowIndex < UI_MAX_WINDOWS; ++WindowIndex) {
        if (UI_GetWindowByID(UI, UI->WindowOrder[WindowIndex]) == Window) {
            CurrentWindowIndex = WindowIndex;
            break;
        }
    }
    
    //Assert(CurrentWindowIndex >= 0);
    
    for (int WindowIndex = 0; WindowIndex < CurrentWindowIndex; ++WindowIndex) {
        ui_window *WindowToTest = UI_GetWindowByID(UI, UI->WindowOrder[WindowIndex]);
        
        v4 CollisionRectToTest = V4(WindowToTest->Position.X,
                                    WindowToTest->Position.Y,
                                    WindowToTest->Position.X + WindowToTest->Size.X,
                                    WindowToTest->Position.Y + WindowToTest->Size.Y);
        
        if (V2InV4(UI->MousePosition, CollisionRectToTest)) {
            return true;
        }
    }
    
    return false;
}

internal ui_widget *
UI_GetWidget(ui *UI, ui_id ID) {
    // Find a widget in the widget list if it exists or else move all remaining elements by one to make space for a
    // new one.
    
    ui_widget *Widget = UI->Widgets + UI->WidgetCount++;
    if (!MatchUIID(Widget->ID, ID)) {
        // @TODO Do some fancy stuff... later
    }
    
    return Widget;
}

internal void
UI_PushPosition(ui *UI, v2 Position) {
    UI->PositionStack[UI->PositionStackCount++] = UI->Position;
    UI->Position += Position;
}

internal void
UI_PushPositionAbsolute(ui *UI, v2 Position) {
    UI->PositionStack[UI->PositionStackCount++] = UI->Position;
    UI->Position = Position;
}

internal void
UI_PopPosition(ui *UI) {
    UI->Position = UI->PositionStack[--UI->PositionStackCount];
}

internal void
UI_PushWidth(ui *UI, real32 Width, bool Auto) {
    UI->WidthStack[UI->WidthStackCount++] = {Width, Auto};
    
    UI->Width = Width;
    UI->AutoWidth = Auto;
}

internal void
UI_PopWidth(ui *UI) {
    --UI->WidthStackCount;
    UI->Width = UI->WidthStack[UI->WidthStackCount].Width;
    UI->AutoWidth = UI->WidthStack[UI->WidthStackCount].AutoWidth;
}

internal void
UI_PushHeight(ui *UI, real32 Height, bool Auto) {
    UI->HeightStack[UI->HeightStackCount++] = {Height, Auto};
    
    UI->Height = Height;
    UI->AutoHeight = Auto;
}

internal void
UI_PopHeight(ui *UI) {
    --UI->HeightStackCount;
    UI->Height = UI->HeightStack[UI->HeightStackCount].Height;
    UI->AutoHeight = UI->HeightStack[UI->HeightStackCount].AutoHeight;
}

internal void
UI_PushRowMode(ui *UI, bool RowMode) {
    UI->RowModeStack[UI->RowModeStackCount++] = UI->RowMode;
    UI->RowMode = RowMode;
}

internal void
UI_PopRowMode(ui *UI) {
    UI->RowMode = UI->RowModeStack[--UI->RowModeStackCount];
}

internal v2
UI_DefaultAutoLayout(ui *UI, v2 *Size) {
    v2 Result = UI->Position;
    
    if (UI->RowMode) {
        UI->Position.X += Size->X;
        
        if (!UI->AutoHeight) {
            // @TODO Move this auto check somewhere else.
            
            Size->Height = UI->Height;
        }
    } else {
        UI->Position.Y += Size->Y;
        
        if (!UI->AutoWidth) {
            Size->Width = UI->Width;
        }
    }
    
    return Result;
}

internal bool
UI_ClickableLogic(ui *UI, ui_id ID, v2 Position, v2 Size) {
    bool Result = false;
    
    v4 CollisionRect = V4(Position.X,
                          Position.Y,
                          Position.X + Size.X,
                          Position.Y + Size.Y);
    
    if (UI->ActiveWindow) {
        CollisionRect = IntersectV4(CollisionRect, MakeRect(UI->ActiveWindow->Position, UI->ActiveWindow->Size));
    }
    
    bool MouseInBounds = V2InV4(UI->MousePosition, CollisionRect);
    
    MouseInBounds &= !UI_TestForOverlappingWindows(UI, UI->ActiveWindow, UI->MousePosition);
    
    if (UI->CurrentDropdown) {
        ui_widget *DropdownWidget = UI->CurrentDropdown;
        
        v4 CollisionRect = MakeRect(V2(DropdownWidget->Position.X,
                                       DropdownWidget->Position.Y + DropdownWidget->Size.Height),
                                    V2(200,
                                       DropdownWidget->Dropdown.CachedFullHeight *
                                       DropdownWidget->Dropdown.OpenTransition));
        MouseInBounds &= V2InV4(UI->MousePosition, CollisionRect);
    }
    
    if (MatchUIID(UI->Active, ID)) {
        if (!UI->LeftMouseDown) {
            Result = MouseInBounds;
            
            UI->Active = UIIDNull();
            if (!MouseInBounds) {
                UI->Hot = UIIDNull();
            }
        }
    } else {
        if (MatchUIID(UI->Hot, ID)) {
            if (UI->LeftMouseDown) {
                UI->Active = ID;
            } else if (!MouseInBounds) {
                UI->Hot = UIIDNull();
            }
        } else {
            if (MatchUIID(UI->Hot, UIIDNull()) && MouseInBounds && !UI->LeftMouseDown) {
                UI->Hot = ID;
            }
        }
    }
    
    return Result;
}

internal v2
UI_DragXY(ui *UI, v2 Position, v2 Size, bool *Clicked = 0) {
    ui_id ID = UI_GenerateID(UI, "Dragable");
    
    bool LogicResult = UI_ClickableLogic(UI, ID, Position, Size);
    
    if (Clicked) {
        *Clicked = LogicResult;
    }
    
    if (MatchUIID(ID, UI->Active)) {
        Position += UI->MousePosition - UI->PreviousMousePosition;
    }
    
    return Position;
}

internal void
UI_AddPadding(ui *UI, real32 Amount) {
    v2 Size = V2(Amount);
    UI->Style.AutoLayout(UI, &Size);
}

internal bool
UI_Button(ui *UI, char *Text) {
    ui_id ID = UI_GenerateID(UI, Text);
    ui_widget *Widget = UI_GetWidget(UI, ID);
    
    Widget->Size = V2(GetTextWidth(UI->Style.Font, Text), GetTextHeight(UI->Style.Font)) +
        V2(UI->Style.Margin * 2, UI->Style.Margin * 2);
    Widget->Position = UI->Style.AutoLayout(UI, &Widget->Size);
    Widget->ID = ID;
    Widget->Type = UIWidget_button;
    Widget->Window = UI->ActiveWindow;
    
    Widget->Text = UI_StoreString(UI, Text);
    
    return UI_ClickableLogic(UI, Widget->ID, Widget->Position, Widget->Size);
}

internal void
UI_Label(ui *UI, char *Text, ...) {
    ui_id ID = UI_GenerateID(UI, Text);
    ui_widget *Widget = UI_GetWidget(UI, ID);
    
    Widget->Size = V2(GetTextWidth(UI->Style.Font, Text), GetTextHeight(UI->Style.Font));
    Widget->Position = UI->Style.AutoLayout(UI, &Widget->Size);
    Widget->ID = ID;
    Widget->Type = UIWidget_label;
    Widget->Window = UI->ActiveWindow;
    
    Widget->Text = UI_StoreString(UI, Text);
}

internal void
UI_FormatLabel(ui *UI, char *Format, ...) {
    char StringBuffer[2048];
    
    va_list Arguments;
    va_start(Arguments, Format);
    
    vsnprintf(StringBuffer, 2048, Format, Arguments);
    
    va_end(Arguments);
    
    UI_Label(UI, StringBuffer);
}

internal void
UI_BeginDropdown(ui *UI, char *Text) {
    ui_id ID = UI_GenerateID(UI, Text);
    ui_widget *Widget = UI_GetWidget(UI, ID);
    
    Widget->Size = V2(GetTextWidth(UI->Style.Font, Text), GetTextHeight(UI->Style.Font)) +
        V2(UI->Style.Margin * 2, UI->Style.Margin * 2);
    Widget->Position = UI->Style.AutoLayout(UI, &Widget->Size);
    Widget->ID = ID;
    Widget->Type = UIWidget_dropdown;
    Widget->Window = UI->ActiveWindow;
    
    Widget->Text = UI_StoreString(UI, Text);
    
    Widget->Dropdown.BelowButton = UI->RowMode;
    
    v4 CollisionRect;
    
    if (Widget->Dropdown.BelowButton) {
        CollisionRect = MakeRect(V2(Widget->Position.X,
                                    Widget->Position.Y + Widget->Size.Height),
                                 V2(200,
                                    Widget->Dropdown.CachedFullHeight *
                                    Widget->Dropdown.OpenTransition));
    } else {
        CollisionRect = MakeRect(V2(Widget->Position.X + Widget->Size.Width,
                                    Widget->Position.Y),
                                 V2(200,
                                    Widget->Dropdown.CachedFullHeight *
                                    Widget->Dropdown.OpenTransition));
    }
    
    bool ButtonIsPressed = UI_ClickableLogic(UI, Widget->ID, Widget->Position, Widget->Size);
    
    if (ButtonIsPressed) {
        Widget->Dropdown.Open = !Widget->Dropdown.Open;
    } else if (IsMousePressed(MouseButton_Left) && !V2InV4(UI->MousePosition, CollisionRect)) {
        Widget->Dropdown.Open = false;
    }
    
    
    // @TODO Figure out how the fuck I'm gonna do the closing properly.
    
    Widget->Dropdown.OpenTransition += ((real32)Widget->Dropdown.Open - Widget->Dropdown.OpenTransition) / 5;
    
    if (Widget->Dropdown.BelowButton) {
        UI_PushPositionAbsolute(UI, Widget->Position + V2(0, Widget->Size.Height));
    } else {
        UI_PushPositionAbsolute(UI, Widget->Position + V2(Widget->Size.Width, 0));
    }
    UI_PushRowMode(UI, false);
    UI_PushWidth(UI, 200, false);
    
    Widget->Dropdown.CachedFullHeight = Widget->Dropdown.FullHeight;
    
    // These lines won't make any sense until later in UI_EndDropdown.
    Widget->Dropdown.WidgetCount = UI->WidgetCount;
    Widget->Dropdown.FullHeight = UI->Position.Y;
    
    UI->CurrentDropdown = Widget;
    
    // @Hack This is only here to disable the collision within window check.
    UI->ActiveWindow = 0;
}

internal void
UI_EndDropdown(ui *UI) {
    ui_widget *Widget = UI->CurrentDropdown;
    Widget->Dropdown.WidgetCount = UI->WidgetCount - Widget->Dropdown.WidgetCount;
    Widget->Dropdown.FullHeight = UI->Position.Y - Widget->Dropdown.FullHeight;
    
    UI_PopWidth(UI);
    UI_PopRowMode(UI);
    UI_PopPosition(UI);
    
    UI->ActiveWindow = Widget->Window;
    
    UI->CurrentDropdown = 0;
}

internal real32
UI_Slider(ui *UI, char *Text, real32 Value, real32 Min, real32 Max) {
    ui_id ID = UI_GenerateID(UI, Text);
    ui_widget *Widget = UI_GetWidget(UI, ID);
    
    Widget->Size = V2(GetTextWidth(UI->Style.Font, Text), GetTextHeight(UI->Style.Font)) +
        V2(UI->Style.Margin * 2, UI->Style.Margin * 2);
    Widget->Position = UI->Style.AutoLayout(UI, &Widget->Size);
    Widget->ID = ID;
    Widget->Type = UIWidget_slider;
    Widget->Window = UI->ActiveWindow;
    
    Widget->Text = UI_StoreString(UI, Text);
    
    UI_ClickableLogic(UI, Widget->ID, Widget->Position, Widget->Size);
    
    if (MatchUIID(UI->Active, Widget->ID)) {
        real32 Scalar = (UI->MousePosition.X - Widget->Position.X) / Widget->Size.Width;
        Widget->SliderRelativeWidth = Scalar;
        
        Value = (Max - Min) * Scalar + Min;
    } else {
        Widget->SliderRelativeWidth = (Value - Min) / (Max - Min);
    }
    
    Widget->SliderRelativeWidth = Clamp(Widget->SliderRelativeWidth, 0, 1);
    Value = Clamp(Value, Min, Max);
    
    return Value;
}

internal bool
UI_Checkbox(ui *UI, char *Text, bool Value) {
    ui_id ID = UI_GenerateID(UI, Text);
    ui_widget *Widget = UI_GetWidget(UI, ID);
    
    Widget->Size = V2(GetTextWidth(UI->Style.Font, Text), GetTextHeight(UI->Style.Font)) +
        V2(UI->Style.Margin * 2, UI->Style.Margin * 2);
    Widget->Position = UI->Style.AutoLayout(UI, &Widget->Size);
    Widget->ID = ID;
    Widget->Type = UIWidget_checkbox;
    Widget->Window = UI->ActiveWindow;
    
    Widget->Text = UI_StoreString(UI, Text);
    
    if (UI_ClickableLogic(UI, Widget->ID, Widget->Position, Widget->Size)) {
        Value = !Value;
    }
    
    Widget->Toggled = Value;
    
    return Value;
}

internal void
UI_Textbox(ui *UI, char *Buffer, int BufferSize) {
    ui_id ID = UI_GenerateID(UI, "Textbox");
    ui_widget *Widget = UI_GetWidget(UI, ID);
    
    Widget->Size = V2(UI->Width, GetTextHeight(UI->Style.Font) + UI->Style.Margin * 2);
    Widget->Position = UI->Style.AutoLayout(UI, &Widget->Size);
    Widget->ID = ID;
    Widget->Type = UIWidget_textbox;
    Widget->Window = UI->ActiveWindow;
    
    Widget->TextBuffer.Base = Buffer;
    Widget->TextBuffer.Size = BufferSize;
    
    if (IsMousePressed(MouseButton_Left) && Platform->KeyboardRedirect && Platform->KeyboardRedirect->Base == Buffer) {
        Platform->KeyboardRedirect = 0;
    }
    
    if (UI_ClickableLogic(UI, Widget->ID, Widget->Position, Widget->Size)) {
        Platform->KeyboardRedirect = &Widget->TextBuffer;
    }
}

internal void
UI_PushWindowOnTop(ui *UI, ui_window *Window) {
    int WindowIndex = 0;
    bool FoundWindow = false;
    
    for (; WindowIndex < UI_MAX_WINDOWS; ++WindowIndex) {
        if (UI_GetWindowByID(UI, UI->WindowOrder[WindowIndex]) == Window) {
            FoundWindow = true;
            break;
        }
    }
    
    if (FoundWindow) {
        if (WindowIndex == 0) {
            return;
        }
        
        memmove(UI->WindowOrder + 1, UI->WindowOrder, (UI_MAX_WINDOWS - WindowIndex) * sizeof(ui_id));
        UI->WindowOrder[0] = Window->ID;
    } else {
        memmove(UI->WindowOrder + 1, UI->WindowOrder, (UI_MAX_WINDOWS - 1) * sizeof(ui_id));
        UI->WindowOrder[0] = Window->ID;
    }
    
    return;
}

internal void
UI_BeginWindow(ui *UI, char *Name, v2 Position, v2 Size, int32 Flags) {
    // This will be a long one, so sit tight.
    
    // Find a window if it exists or make space for a new one.
    ui_window *Window = UI->Windows + UI->WindowCount;
    
    // Technically if a collision occurs with another ID the windows persistent data will be lost. Although the odds
    // of this happening is 1 / UI_MAX_SECONDARY_IDENTIFIERS which means that this has a 0.1% chance of happening.
    ui_id ID = UI_GenerateID(UI, Name);
    
    // If the ID's already match, great!
    if (!MatchUIID(Window->ID, ID)) {
        bool FoundWindow = false;
        
        // But alas they didn't, although maybe it exists in the list of windows?
        for (int WindowIndex = UI->WindowCount + 1; WindowIndex < UI_MAX_WINDOWS; ++WindowIndex) {
            if (MatchUIID(ID, UI->Windows[WindowIndex].ID)) {
                ui_window TemporaryWindow = UI->Windows[WindowIndex];
                
                memmove(Window + 1, Window, (WindowIndex - UI->WindowCount) * sizeof(ui_window));
                *Window = TemporaryWindow;
                
                FoundWindow = true;
                break;
            }
        }
        
        if (!FoundWindow) {
            // The window didn't exist so we'll simply move all other windows to make room for it.
            memmove(UI->Windows + 1, UI->Windows, (UI_MAX_WINDOWS - 1) * sizeof(ui_window));
            
            // Window Init Code.
            Window->ID = ID;
            Window->Position = Position;
            Window->Size = Size;
            Window->Flags = Flags;
            
            strncpy(Window->Name, Name, 60);
            
            UI_PushWindowOnTop(UI, Window);
        }
    }
    
    UI->ActiveWindow = Window;
    ++UI->WindowCount;
    
    Window->StartWidgetIndex = UI->WidgetCount;
    
    if (MatchUIID(UI->Hot, UIIDNull())){
        v4 CollisionRect = V4(Window->Position.X,
                              Window->Position.Y,
                              Window->Position.X + Window->Size.X,
                              Window->Position.Y + Window->Size.Y);
        
        bool MouseInBounds = V2InV4(UI->MousePosition, CollisionRect);
        
        if (UI->DraggedWindow == 0) {
            // @TODO Check that this window is on top of any other potential window.
            // Something like this... (note, I wrote this while tired in a test code sort of way so don't expect it
            // to be perfect.)
            
            if (UI->LeftMouseDown) {
                v4 CollisionRect = V4(Window->Position.X,
                                      Window->Position.Y,
                                      Window->Position.X + Window->Size.X,
                                      Window->Position.Y + Window->Size.Y);
                
                bool MouseInBounds = V2InV4(UI->MousePosition, CollisionRect);
                
                if (MouseInBounds) {
                    if (!UI_TestForOverlappingWindows(UI, Window, UI->MousePosition)) {
                        UI_PushWindowOnTop(UI, Window);
                        UI->DraggedWindow = Window;
                    }
                }
            }
            
        }
        
        if (UI->DraggedWindow == Window) {
            if (UI->LeftMouseDown) {
                Window->Position -= UI->PreviousMousePosition - UI->MousePosition;
            } else {
                UI->DraggedWindow = 0;
            }
        }
    }
    
    if (Flags & UI_WINDOW_FLAG_TITLEBAR) {
        // Add titlebar widgets
        UI_PushPositionAbsolute(UI, Window->Position + V2(UI->Style.Margin, UI->Style.Margin));
        UI_Label(UI, Window->Name);
        UI_PopPosition(UI);
        
        
        Window->TitlebarHeight = GetTextHeight(UI->Style.Font) + UI->Style.Margin * 2;
        
        UI_PushPositionAbsolute(UI, Window->Position + V2(UI->Style.Margin, UI->Style.Margin + Window->TitlebarHeight));
    } else {
        UI_PushPositionAbsolute(UI, Window->Position + V2(UI->Style.Margin, UI->Style.Margin));
    }
    
    if (Flags & UI_WINDOW_FLAG_FORCE_SIZE) {
        UI_PushWidth(UI, Window->Size.Width - UI->Style.Margin * 2, false);
    }
    
    UI_PushRowMode(UI, false);
}

internal void
UI_EndWindow(ui *UI) {
    ui_window *Window = UI->ActiveWindow;
    
    Window->EndWidgetIndex = UI->WidgetCount;
    UI->ActiveWindow = 0;
    
    if (Window->Flags & UI_WINDOW_FLAG_FORCE_SIZE) {
        UI_PopWidth(UI);
    }
    
    UI_PopPosition(UI);
    UI_PopRowMode(UI);
}

internal void
UI_DefaultStyle(ui_style *Style, font *Font) {
    Style->Margin = 5;
    Style->Font = Font;
    Style->RenderWidget = UI_DefaultRenderWidget;
    Style->AutoLayout = UI_DefaultAutoLayout;
}