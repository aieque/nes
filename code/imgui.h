#define UI_MAX_WIDGETS 1000
#define UI_MAX_SECONDARY_IDENTIFIERS 1000

struct ui_id {
    uint32 Primary;
    uint32 Secondary;
};

#define UI_WINDOW_FLAG_TITLEBAR   (1 << 0)
#define UI_WINDOW_FLAG_FORCE_SIZE (1 << 1)

#define UI_MAX_WINDOWS 32

struct ui_window {
    ui_id ID;
    v2 Position;
    v2 Size;
    
    char Name[60]; // If I change the way text is stored in widgets, this should change too.
    
    int32 Flags;
    
    uint32 StartWidgetIndex;
    uint32 EndWidgetIndex; 
    
    real32 TitlebarHeight;
    
    bool Collapsed;
};

enum ui_widget_type {
    UIWidget_button,
    UIWidget_label,
    UIWidget_dropdown,
    UIWidget_checkbox,
    UIWidget_slider,
    UIWidget_textbox,
};

struct ui_widget {
    ui_id ID;
    
    ui_window *Window;
    
    v2 Position;
    v2 Size;
    
    // Used for fade-in effects during hot and active.
    real32 HotTransition;
    real32 ActiveTransition;
    
    ui_widget_type Type;
    
    char *Text;
    
    bool Toggled;
    real32 ToggleTransition;
    
    // Bad name for the amount of percent the width is multiplied by when displaying the slider.
    real32 SliderRelativeWidth;
    
    string_buffer TextBuffer;
    
    struct {
        bool Open;
        real32 OpenTransition;
        
        int WidgetCount;
        
        bool BelowButton;
        
        real32 FullHeight;
        real32 CachedFullHeight; // Used for collision checks.
    } Dropdown;
};

struct ui;
struct renderer;

typedef int ui_render_widget(ui *UI, ui_widget *Widget, renderer *Renderer);
typedef v2 ui_auto_layout(ui *UI, v2 *Size);

struct ui_style {
    real32 Margin;
    
    font *Font;
    
    ui_render_widget *RenderWidget;
    ui_auto_layout *AutoLayout;
};

#define UI_STACK_SIZE 32

struct ui {
    ui_id Hot;
    ui_id Active;
    
    int SecondaryIdentifier[UI_MAX_SECONDARY_IDENTIFIERS];
    
    ui_style Style;
    
    // @TODO Maybe turn this into a memory arnea???
    char StringBuffer[2048];
    int StringBufferUsed;
    
    ui_window *ActiveWindow;
    ui_window Windows[UI_MAX_WINDOWS];
    int WindowCount;
    ui_id WindowOrder[UI_MAX_WINDOWS];
    ui_window *DraggedWindow;
    
    ui_widget *CurrentDropdown;
    
    bool LeftMouseDown;
    v2 MousePosition;
    v2 PreviousMousePosition;
    
    ui_id UsedID[UI_MAX_WIDGETS];
    int UsedIDCount;
    
    ui_widget Widgets[UI_MAX_WIDGETS];
    int WidgetCount;
    int PreviousWidgetCount;
    
    
    v2 Position;
    bool RowMode;
    
    real32 Width;
    real32 Height;
    
    bool AutoWidth;
    bool AutoHeight;
    
    
    v2 PositionStack[UI_STACK_SIZE];
    int PositionStackCount;
    
    struct {
        real32 Width;
        bool AutoWidth;
    } WidthStack[UI_STACK_SIZE];
    int WidthStackCount;
    
    struct {
        real32 Height;
        bool AutoHeight;
    } HeightStack[UI_STACK_SIZE];
    int HeightStackCount;
    
    bool RowModeStack[UI_STACK_SIZE];
    int RowModeStackCount;
};
