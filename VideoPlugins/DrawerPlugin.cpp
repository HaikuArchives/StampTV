/*

	DrawerPlugin.cpp

	© 2001, Georges-Edouard Berenger, All Rights Reserved.
	See the StampTV Video plugin License for usage restrictions.

*/

#include "DrawerPlugin.h"

#include <Bitmap.h>
#include <stdio.h>
#include <View.h>
#include <Message.h>
#include <TextControl.h>

status_t GetVideoPlugins(TList<VideoPluginType> & plugin_list, VideoPluginsHost * host, image_id id)
{
	status_t compatible = host->APIVersionCompatible();
	if (compatible != B_OK)
		return compatible;

	plugin_list.AddItem(new DrawerPluginType(host, id));
	return B_OK;
}

DrawerPluginType::DrawerPluginType(VideoPluginsHost * host, image_id id) :
	VideoPluginType(host, id)
{
}

DrawerPluginType::~DrawerPluginType()
{
}

const char* DrawerPluginType::ID()
{
	return "Drawer";
}

uint32 DrawerPluginType::Flags()
{
	return VAPI_CONFIGURABLE;
}

uint32 DrawerPluginType::Version()
{
	return 001;
}

const char* DrawerPluginType::Name()
{
	return "Drawer";
}

const char* DrawerPluginType::Author()
{
	return "Geb";
}

static const vertice_properties support[] =
{
	{	ANY_COLOR_SPACE,
		ANY_COLOR_SPACE,
		1.0,
		VAPI_PROCESS_IN_PLACE | VAPI_BVIEW_DRAWING,
	},
};

int32 DrawerPluginType::GetVerticesProperties(const vertice_properties ** ap)
{
	*ap = support;
	return sizeof(support) / sizeof(vertice_properties);
}

VideoPluginEngine * DrawerPluginType::InstanciateVideoPluginEngine()
{
	return new DrawerEngine(this);
}

//-------------------------------------------------------------------------
// This view is used as center of configuration.
// It handles the controls in it, and forwards the changes to the engine
class ConfigView : public BView {
	public:
							ConfigView(DrawerEngine * engine, BRect frame);
	virtual	void			AttachedToWindow();
	virtual	void			DetachedFromWindow();
	virtual	void			MessageReceived(BMessage *msg);
			void			Update();

	private:
		DrawerEngine *		fEngine;
		BTextControl *		fTextControl;
};

ConfigView::ConfigView(DrawerEngine * engine, BRect frame) :
	BView(frame, B_EMPTY_STRING, B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW), fEngine(engine), fTextControl(NULL)
{
}

void ConfigView::AttachedToWindow()
{
	BRect	frame = Bounds();
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	frame.InsetBy(20, 20);
	BString	text;
	fEngine->GetText(text);
	BMessage *	m = new BMessage('text');
	fTextControl = new BTextControl(frame, B_EMPTY_STRING, "Texte:", text.String(), m, B_FOLLOW_ALL);
	fTextControl->SetDivider(40);
	AddChild(fTextControl);
	fTextControl->SetTarget(this);
}

void ConfigView::DetachedFromWindow()
{
	Update();
	fEngine->SetConfigWindow(NULL);
}

void ConfigView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case 'text':
			Update();
			break;
		default:
			BView::MessageReceived(msg);
	}
}

void ConfigView::Update()
{
	if (fTextControl) {
		fEngine->SetText(fTextControl->Text());
	}
}

//-------------------------------------------------------------------------

DrawerEngine::DrawerEngine(VideoPluginType * plugin) : VideoPluginEngine(plugin), fText("Hello World!"), fFont(be_bold_font)
{
	fFont.SetSize(48);
	fFont.SetFlags(fFont.Flags() | B_DISABLE_ANTIALIASING);
}

DrawerEngine::~DrawerEngine()
{
}

status_t DrawerEngine::RestoreConfig(BMessage & config)
{
	const char * text;
	if (config.FindString("text", &text) == B_OK)
		SetText(text);
	return B_OK;
}

status_t DrawerEngine::StoreConfig(BMessage & config)
{
	if (fLock.Lock() == B_OK) {
		config.AddString("text", fText.String());
		fLock.Unlock();
	}
	return B_OK;
}

BView* DrawerEngine::ConfigureView()
{
	return new ConfigView(this, BRect(0, 0, 200, 50));
}

status_t DrawerEngine::UseFormat(media_raw_video_format & format, int which_work_mode)
{
	return B_OK;
}

// Note that access to the parameters need to be made thread safe *only* because we have a configuration window.
// If you need to access more than one member in the ApplyEffect method, then you should lock once at the begining,
// and unlock once at the end of processing. The accesses made by the configuration window are not much time constrained
// and may be delayed for a good while, while the image processing shouldn't, which means that the configuration
// window should lock with the finest granularity.

status_t DrawerEngine::ApplyEffect(BBitmap * frame, BBitmap * frame_out, int64 frame_count, bool skipped_frames)
{
	if (frame != frame_out)
		return B_ERROR; // a view allows you to draw only in out bitmap...
	BView * view = frame->ChildAt(0);
	if (!view)
		return B_ERROR;
	// The view is set: Let's draw!
	view->SetFont(&fFont);
	view->SetHighColor(255, 0, 0);
	if (fLock.Lock() == B_OK) {
		view->DrawString(fText.String(), BPoint(20, 120));
		fLock.Unlock();
	}
	
	// It is your duty to make so that the view's frame rect is the same as the bitmap.
	// If you move/resize the view, then reset the view position & size before you return!
	// The same goes for clipping: if you set it, clear it! It is *your* job!
	// This policy allows plugins to avoid unecessary tests.
	return B_OK;
}

void DrawerEngine::SetText(const char * text)
{
	if (fLock.Lock() == B_OK) {
		fText = text;
		fLock.Unlock();
	}
}

void DrawerEngine::GetText(BString	& text)
{
	if (fLock.Lock() == B_OK) {
		text = fText;
		fLock.Unlock();
	}
}
