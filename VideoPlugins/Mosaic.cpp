/*

	Mosaic.cpp

	© 2001, Georges-Edouard Berenger, All Rights Reserved.
	See the StampTV Video plugin License for usage restrictions.

*/

#include "Mosaic.h"

#include <Bitmap.h>
#include <stdio.h>
#include <View.h>
#include <string.h>
#include <Region.h>
#include <String.h>
#include <Slider.h>
#include <Box.h>
#include <CheckBox.h>
#include <Button.h>
#include <StringView.h>
#include <File.h>
#include <FindDirectory.h>
#include <RadioButton.h>

status_t GetVideoPlugins(TList<VideoPluginType> & plugin_list, VideoPluginsHost * host, image_id id)
{
	status_t compatible = host->APIVersionCompatible();
	if (compatible != B_OK)
		return compatible;

	plugin_list.AddItem(new MosaicPluginType(host, id));
	return B_OK;
}

MosaicPluginType::MosaicPluginType(VideoPluginsHost * host, image_id id) :
	VideoPluginType(host, id)
{
}

MosaicPluginType::~MosaicPluginType()
{
}

const char* MosaicPluginType::ID()
{
	return "Mosaic";
}

uint32 MosaicPluginType::Flags()
{
	return VAPI_CONFIGURABLE;
}

uint32 MosaicPluginType::Version()
{
	return 001;
}

const char* MosaicPluginType::Name()
{
	return "Mosaic";
}

const char* MosaicPluginType::Author()
{
	return "Geb";
}

static const vertice_properties support[] =
{
	{	ANY_COLOR_SPACE,
		ANY_COLOR_SPACE,
		1.0,
		VAPI_PROCESS_IN_PLACE | VAPI_PROCESS_TO_DISTINCT | VAPI_PROCESS_TO_OVERLAY,
	},
};

int32 MosaicPluginType::GetVerticesProperties(const vertice_properties ** ap)
{
	*ap = support;
	return sizeof(support) / sizeof(vertice_properties);
}

VideoPluginEngine * MosaicPluginType::InstanciateVideoPluginEngine()
{
	return new MosaicEngine(this);
}

//-------------------------------------------------------------------------
// Slider view with current value displayed on the top right position...
class ValueSlider : public BSlider
{
public:
					ValueSlider(BRect frame, const char * label, int32 min, int32 max, thumb_style thumbType,
						BMessage * m = NULL, const char* unit = B_EMPTY_STRING);

	void			AttachedToWindow();
	void			SetValue(int32 v);
	void			SetVariable(int32 * variable);

private:
	int32 *			fVariable;
	BStringView *	fDisplay;
	BString			fUnit;
};

ValueSlider::ValueSlider (BRect frame, const char * label, int32 min, int32 max, thumb_style thumbType, BMessage * m, const char* unit)
	:BSlider (frame, B_EMPTY_STRING, label, m, min, max, thumbType), fVariable(NULL), fDisplay(NULL), fUnit(unit)
{
	BString	left;
	BString	right;
	left << min;
	right << max;
	SetLimitLabels(left.String(), right.String());
}
					
void ValueSlider::AttachedToWindow()
{
	BSlider::AttachedToWindow();
	BRect	frame = Bounds();
	frame.left = frame.right - 60;
	frame.bottom = frame.top + 13;
	fDisplay = new BStringView (frame, NULL, "");
	AddChild (fDisplay);
	fDisplay->SetAlignment (B_ALIGN_RIGHT);
}

void ValueSlider::SetValue(int32 v)
{
	BSlider::SetValue(v);
	v = Value();
	if (fDisplay) {
		char	label[256];
		sprintf (label, "%d%s", int (v), fUnit.String());
		fDisplay->SetText (label);
	}
	if (fVariable)
		*fVariable = v;
}

void ValueSlider::SetVariable(int32 * variable)
{
	if (variable)
		SetValue(*variable);
	fVariable = variable;
}

//-------------------------------------------------------------------------
// This view is used as center of configuration.
// It handles the controls in it, and forwards the changes to the engine
class ConfigView : public BView {
	public:
							ConfigView(MosaicEngine * engine);
	virtual	void			AttachedToWindow();
	virtual	void			DetachedFromWindow();
	virtual	void			MessageReceived(BMessage *msg);
			void			RefreshUI();

	private:
		MosaicEngine *		fEngine;
		ValueSlider *		fCountSlider;
		ValueSlider *		fDelaySlider;
		BCheckBox *			fDisplayTitle;
		BCheckBox *			fThough;
		BRadioButton *		fScanPresets;
		BRadioButton *		fScanChannels;
		BRadioButton *		fNoScan;
};

ConfigView::ConfigView(MosaicEngine * engine) :
	BView(BRect(0, 0, 510, 210), B_EMPTY_STRING, B_FOLLOW_NONE, B_WILL_DRAW), fEngine(engine)
{
}

void ConfigView::AttachedToWindow()
{
	const float	kHSideDist = 20;
	const float kVSideDist = 15;
	const float kHDist = 10;
	const float kVDist = 8;
	const float kButtonWidth = 110;
	
	BRect	frame = Bounds();
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BRect	rect;
	rect.Set(0, 0, 300, 10);
	rect.OffsetTo(kHSideDist, kVSideDist);
	fCountSlider = new ValueSlider(rect, "Count", 1, 20, B_TRIANGLE_THUMB);
	fCountSlider->SetBarThickness(5);
	AddChild(fCountSlider);
	fCountSlider->SetTarget(this);
	
	rect.OffsetBy(0, fCountSlider->Frame().Height() + kVDist);
	fDelaySlider = new ValueSlider(rect, "Delay", 1, 300, B_TRIANGLE_THUMB, NULL, " Frames");
	fDelaySlider->SetBarThickness(5);
	AddChild(fDelaySlider);
	fDelaySlider->SetTarget(this);
	
	rect.OffsetBy(0, fDelaySlider->Frame().Height() + kVDist);
	rect.right = rect.left + rect.Width() / 2;
	fDisplayTitle = new BCheckBox(rect, B_EMPTY_STRING, "Display Channel's Name", new BMessage('Titl'));
	AddChild(fDisplayTitle);
	fDisplayTitle->SetTarget(this);
	
	rect.OffsetBy(rect.Width(), 0);
	fThough = new BCheckBox(rect, B_EMPTY_STRING, "Zoom to Current", new BMessage('Thro'));
	AddChild(fThough);
	fThough->SetTarget(this);
	
	rect.Set(rect.right + kHDist * 2, kVSideDist, frame.right - kHSideDist, fDisplayTitle->Frame().bottom);
	BBox * box = new BBox(rect);
	AddChild(box);
	rect = box->Bounds();
	BRect	r(kHSideDist, kVSideDist * 2, rect.right - kHSideDist, 0);
	fScanPresets = new BRadioButton(r, B_EMPTY_STRING, "Scan Presets", new BMessage('SPre'));
	box->AddChild(fScanPresets);
	fScanPresets->SetTarget(this);
	r.OffsetBy(0, fScanPresets->Frame().Height() + kVDist * 2);
	fScanChannels = new BRadioButton(r, B_EMPTY_STRING, "Scan Channels", new BMessage('SCha'));
	box->AddChild(fScanChannels);
	fScanChannels->SetTarget(this);

	r.OffsetBy(0, fScanPresets->Frame().Height() + kVDist * 2);
	fNoScan = new BRadioButton(r, B_EMPTY_STRING, "Stick to Channel", new BMessage('NoSc'));
	box->AddChild(fNoScan);
	fNoScan->SetTarget(this);
	box->SetLabel("Scan type:");

	rect.Set(0, 0, kButtonWidth, 20);
	BButton *	button = new BButton(rect, B_EMPTY_STRING, "Factory Settings", new BMessage('Fact'));
	AddChild(button);	// button resize when attached to a window: let's use its natural height
	button->SetTarget(this);
	float	y = frame.bottom - button->Frame().Height() - kVSideDist;
	button->MoveTo(kHSideDist, y);
	rect.OffsetTo(kHSideDist + (kButtonWidth + kHDist) * 1, y);
	AddChild(button = new BButton(rect, B_EMPTY_STRING, "Default Settings", new BMessage('Defa')));
	button->SetTarget(this);
	rect.OffsetTo(kHSideDist + (kButtonWidth + kHDist) * 2, y);
	AddChild(button = new BButton(rect, B_EMPTY_STRING, "Set as Default", new BMessage('SDef')));
	button->SetTarget(this);
	rect.OffsetTo(kHSideDist + (kButtonWidth + kHDist) * 3, y);
	AddChild(button = new BButton(rect, B_EMPTY_STRING, "Close", new BMessage(B_QUIT_REQUESTED)));

	RefreshUI();
}

void ConfigView::DetachedFromWindow()
{
	fEngine->SetConfigWindow(NULL);
}

void ConfigView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case 'Titl':
			fEngine->mDisplayTitle = fDisplayTitle->Value() != 0;
			break;
		case 'Thro': {
			fEngine->mThrough = fThough->Value() != 0;
			fEngine->UpdateMute();
			break;
		}
		case 'SPre':
				fEngine->mScanType = 0;
				fEngine->UpdateMute();
			break;
		case 'SCha':
				fEngine->mScanType = 1;
				fEngine->UpdateMute();
			break;
		case 'NoSc':
				fEngine->mScanType = 2;
				fEngine->UpdateMute();
			break;
		case 'SDef': {
			fEngine->mDefaults.MakeEmpty();
			fEngine->StoreConfig(fEngine->mDefaults);
			BFile	prefs(fEngine->mDefaultsPath.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
			if (prefs.InitCheck() == B_OK)
				fEngine->mDefaults.Flatten(&prefs);
			break;
		}
		case 'Defa': {
			BMessage	empty;
			fEngine->RestoreConfigWithDefault(fEngine->mDefaults, empty);
			RefreshUI();
			break;
		}
		case 'Fact': {
			BMessage	empty;
			fEngine->RestoreConfigWithDefault(empty, empty);
			RefreshUI();
			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}

void ConfigView::RefreshUI()
{
	fCountSlider->SetVariable(&fEngine->mCount);
	fDelaySlider->SetVariable(&fEngine->mDelay);
	fDisplayTitle->SetValue(fEngine->mDisplayTitle);
	fThough->SetValue(fEngine->mThrough);
	if (fEngine->mScanType == 0)
		fScanPresets->SetValue(1);
	else if (fEngine->mScanType == 1)
		fScanChannels->SetValue(1);
	else
		fNoScan->SetValue(1);
}

//-------------------------------------------------------------------------

MosaicEngine::MosaicEngine(VideoPluginType * plugin) : VideoPluginEngine(plugin)
{
	Host()->SetWindowTitle("Mosaic");
	mMosaic = NULL;
	find_directory(B_USER_SETTINGS_DIRECTORY, &mDefaultsPath, true);
	mDefaultsPath.Append("Mosaic Plugin Preferences");
	BFile	prefs(mDefaultsPath.Path(), B_READ_ONLY);
	if (prefs.InitCheck() == B_OK)
		mDefaults.Unflatten(&prefs);
	RestoreConfig(mDefaults);
}

MosaicEngine::~MosaicEngine()
{
	Host()->SetWindowTitle();
	Host()->SetMuteAudio(false);
	if (mMosaic->Lock())
		delete mMosaic;
}

status_t MosaicEngine::RestoreConfig(BMessage & config)
{
	RestoreConfigWithDefault(config, mDefaults);
	UpdateMute();
	return B_OK;
}

status_t MosaicEngine::StoreConfig(BMessage & config)
{
	config.AddInt32("count", mCount);
	config.AddInt32("delay", mDelay);
	config.AddBool("displayTitle", mDisplayTitle);
	config.AddInt32("scantype", mScanType);
	return B_OK;
}

void MosaicEngine::RestoreConfigWithDefault(BMessage & config, BMessage & defaults)
{
	// restores config from config, if not defaults, if not using factory setting...
	if (config.FindInt32("count", &mCount) != B_OK && defaults.FindInt32("count", &mCount) != B_OK)
		mCount = 5;
	if (config.FindInt32("delay", &mDelay) != B_OK && defaults.FindInt32("delay", &mDelay) != B_OK)
		mDelay = 5;
	if (config.FindBool("displayTitle", &mDisplayTitle) != B_OK && defaults.FindBool("displayTitle", &mDisplayTitle) != B_OK)
		mDisplayTitle = true;
	if (config.FindInt32("scantype", &mScanType) != B_OK && defaults.FindInt32("scantype", &mScanType) != B_OK)
		mScanType = 0;
	mThrough = false;
}

BView* MosaicEngine::ConfigureView()
{
	return new ConfigView(this);
}

status_t MosaicEngine::UseFormat(media_raw_video_format & format, int which_work_mode)
{
	const rgb_color kDisplayColor = {0, 203, 0, 255};

	mWidth = format.display.line_width;
	mHeight = format.display.line_count;
	if (mMosaic && mMosaic->Lock())
		delete mMosaic;
	BRect	frame(0, 0, mWidth - 1, mHeight - 1);
	mMosaic = new BBitmap(frame, format.display.format, true);
	mMosaic->Lock();
	mView = new BView(frame, B_EMPTY_STRING, 0, B_WILL_DRAW);
	mMosaic->AddChild(mView);
	memset(mMosaic->Bits(), 0, mMosaic->BitsLength());
	mView->SetHighColor(kDisplayColor);
	mMosaic->Unlock();
	mLastFrame = -10000;
	mPos = 0;
	mLastCount = -1;
	return B_OK;
}

status_t MosaicEngine::ApplyEffect(BBitmap * frame_in, BBitmap * frame_out, int64 frame_count, bool skipped_frames)
{
	if (!mMosaic)
		return B_OK;
	int64 * S;
	if (!mThrough) {
		if (frame_count > mLastFrame + mDelay) {
			mMosaic->Lock();
			if (mCount != mLastCount) {
				mLastCount = mCount;
				BFont	font(be_bold_font);
				mFSize = floorf(mHeight * 0.15 / mLastCount);
				mFSize = mFSize < 9 ? 9 : mFSize;
				font.SetSize(mFSize);
				font.SetFlags(font.Flags() | B_DISABLE_ANTIALIASING);
				mView->SetFont(&font);
			}
			mLastFrame = frame_count;
			float	x = floor(mWidth / mLastCount);
			float	y = floor(mHeight/ mLastCount);
			int		px = mPos % mLastCount;
			int		py = mPos / mLastCount;
			float	mTx = floor(mWidth * 0.05 / mLastCount);
			float	mTy = floor(mFSize);
			BRect	pic(0, 0, x - 1.0, y - 1.0);
			pic.OffsetBy(px * x, py * y);
			BRegion	region(pic);
			mView->ConstrainClippingRegion(&region);
			if (mLastCount > 1) {
				const float	zoomFactor = 0.005 * mLastCount;
				float	cutW = floor(mWidth * zoomFactor);
				float	cutH = floor(mHeight * zoomFactor);
				mView->DrawBitmapAsync(frame_in, BRect(cutW, cutH, mWidth - cutW, mHeight - cutH), pic);
			} else
				mView->DrawBitmapAsync(frame_in, pic);
			int32	slot;
			if (mDisplayTitle) {
				BString	name;
				int32	channel;
				Host()->GetPreset(slot, name, channel);
				mView->DrawString(name.String(), BPoint(pic.left + mTx, pic.top + mTy));
			}
			mView->Sync();
			mMosaic->Unlock();
			int32	index;
			if (mScanType == 0) {
				if (Host()->SwitchToNext(1, true, index) != B_OK)
					Host()->SwitchToPreset(0);
			} else if (mScanType == 1) {
				if (Host()->SwitchToNext(1, false, index) != B_OK)
					Host()->SwitchToChannel(0);
			}
			// DO NOT USE memcpy!!! IT IS  *H*O*R*R*R*R*I*B*L*Y*  SLOW WITH OVERLAYS!!!
			if (++mPos >= mLastCount * mLastCount)
				mPos = 0;
		}
		S = (int64*) mMosaic->Bits();
	} else
		S = (int64*) frame_in->Bits();
	int64 * D = (int64*) frame_out->Bits();
	if (!(mThrough && S == D)) {
		int l = mMosaic->BitsLength();
		while (l >= 16 * 8) {
			*D = *S; D[1] = S[1]; D[2] = S[2]; D[3] = S[3]; D[4] = S[4]; D[5] = S[5]; D[6] = S[6]; D[7] = S[7];
			D[8] = S[8]; D[9] = S[9]; D[10] = S[10]; D[11] = S[11]; D[12] = S[12]; D[13] = S[13]; D[14] = S[14]; D[15] = S[15];
			S += 16; D += 16; l -= 16 * 8;
		}
		while (l >= 4 * 8) {
			*D = *S; D[1] = S[1]; D[2] = S[2]; D[3] = S[3];
			S += 4; D += 4; l -= 4 * 8;
		}
		while (l >= 8) {
			*D = *S;
			S += 1; D += 1; l -= 8;
		}
		if (l > 0) {
			int32 * s4 = (int32*) S;
			int32 * d4 = (int32*) D;
			if (l >= 4) {
				*d4++ = *s4++;
				l -= 4;
			}
			if (l >= 2) {
				int16 * s2 = (int16*) s4;
				int16 * d2 = (int16*) d4;
				*d2 = *s2;
			}
		}
	}
	return B_OK;
}

void MosaicEngine::UpdateMute()
{
	Host()->SetMuteAudio(!mThrough && mScanType < 2);
}