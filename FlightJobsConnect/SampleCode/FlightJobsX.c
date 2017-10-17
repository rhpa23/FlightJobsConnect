/*
* FlightJobsX.c
*
* This plugin implements the canonical first program.  In this case, we will
* create a window that has the text hello-world in it.  As an added bonus
* the  text will change to 'This is a plugin' while the mouse is held down
* in the window.
*
* This plugin demonstrates creating a window and writing mouse and drawing
* callbacks for that window.
*
*/

#include <stdio.h>
#include <string.h>
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMNavigation.h"
#include "XPLMDataAccess.h"
#include "XPUIGraphics.h"
#include "XPLMMenus.h"

#include "XPWidgets.h"
#include "XPStandardWidgets.h"

#include <curl/curl.h>
#include <stdlib.h>

/*
* Global Variables.  We will store our single window globally.  We also record
* whether the mouse is down from our mouse handler.  The drawing handler looks
* at this information and draws the appropriate display.
*
*/

// Widget prototypes
void CreateWidgetWindow(int x1, int y1, int w, int h);
void UpdateData();
int login(int num);

XPWidgetID	FlightJobsXWidget = NULL, FlightJobsXWindow = NULL;
XPWidgetID	RefreshXButton, LoginXButton, StartXButton = NULL;
XPWidgetID	LocationText;
XPWidgetID	MessageCaption, UserNameCaption, UserNameTextBox, PassWordCaption, PassWordTextBox = NULL;
char FlightJobsXVersionNumber[] = "v1.00";

// Menu Prototypes
void FlightJobsXMenuHandler(void *, void *);
void FlightJobsXMenuHandler2(void *, void *);

int FlightJobsXMenuItem1;
int FlightJobsXIsStarted;
int	FlightJobsXMenuItem = 0, FlightJobsXMenuItem2 = 0;
XPLMMenuID	FlightJobsXMenuId = NULL, FlightJobsXMenuId2 = NULL;

#define FlightJobsX_MAX_ITEMS 5

int FlightJobsXHandler(
	XPWidgetMessage			inMessage,
	XPWidgetID				inWidget,
	intptr_t				inParam1,
	intptr_t				inParam2);

float	MyFlightLoopCallback(
	float                inElapsedSinceLastCall,
	float                inElapsedTimeSinceLastFlightLoop,
	int                  inCounter,
	void *               inRefcon);



/*
* XPluginStart
*
* Our start routine registers our window and does any other initialization we
* must do.
*
*/
PLUGIN_API int XPluginStart(
	char *		outName,
	char *		outSig,
	char *		outDesc)
{
	/* First we must fill in the passed in buffers to describe our
	* plugin to the plugin-system. */

	strcpy(outName, "FlightJobsX");
	strcpy(outSig, "xplanesdk.examples.helloworld");
	strcpy(outDesc, "A plugin that start pending jobs in FlightJobs system.");

	FlightJobsXMenuItem1 = 0;
	FlightJobsXIsStarted = 0;
	// Create the menus
	FlightJobsXMenuItem = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "FlightJobsX", NULL, 1);
	FlightJobsXMenuId = XPLMCreateMenu("FlightJobsX", XPLMFindPluginsMenu(), FlightJobsXMenuItem, FlightJobsXMenuHandler, NULL);
	FlightJobsXMenuItem2 = XPLMAppendMenuItem(FlightJobsXMenuId, "FlightJobsX", (void *)"FlightJobsX", 1);

//	CreateWidgetWindow(100, 550, 650, 330);
	/* We must return 1 to indicate successful initialization, otherwise we
	* will not be called back again. */

	/* Register our callback for once a second.  Positive intervals
	* are in seconds, negative are the negative of sim frames.  Zero
	* registers but does not schedule a callback for time. */
	XPLMRegisterFlightLoopCallback(
		MyFlightLoopCallback,	/* Callback */
		-1.0,					/* Interval */
		NULL);

	return 1;
}

/*
* XPluginStop
*
* Our cleanup routine deallocates our window.
*
*/
PLUGIN_API void	XPluginStop(void)
{
	XPLMUnregisterFlightLoopCallback(MyFlightLoopCallback, NULL);
	XPLMDestroyMenu(FlightJobsXMenuId);
	// Clean up
}

/*
* XPluginDisable
*
* We do not need to do anything when we are disabled, but we must provide the handler.
*
*/
PLUGIN_API void XPluginDisable(void)
{
}

/*
* XPluginEnable.
*
* We don't do any enable-specific initialization, but we must return 1 to indicate
* that we may be enabled at this time.
*
*/
PLUGIN_API int XPluginEnable(void)
{
	return 1;
}

/*
* XPluginReceiveMessage
*
* We don't have to do anything in our receive message handler, but we must provide one.
*
*/
PLUGIN_API void XPluginReceiveMessage(
	XPLMPluginID	inFromWho,
	long			inMessage,
	void *			inParam)
{
}


float	MyFlightLoopCallback(
	float                inElapsedSinceLastCall,
	float                inElapsedTimeSinceLastFlightLoop,
	int                  inCounter,
	void *               inRefcon)
{
	
	UpdateData();
	return 1.0;

}

void UpdateData()
{
	/* The actual callback.  First we read the sim's time and the data. */
	float	elapsed = XPLMGetElapsedTime();

	float	color[] = { 0.0, 0.0, 0.0 }; 	/* RGB Black */

											/* First find the plane's position. */
	float lat = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/latitude"));
	float lon = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/longitude"));
	char  airportInfo[256];

	/* Find the nearest airport to us. */
	XPLMNavRef	ref = XPLMFindNavAid(NULL, NULL, &lat, &lon, NULL, xplm_Nav_Airport);
	if (ref != XPLM_NAV_NOT_FOUND)
	{
		/* If we found one, get its information, and say it. */
		char	id[10];
		char	name[256];
		XPLMGetNavAidInfo(ref, NULL, &lat, &lon, NULL, NULL, NULL, id, name, NULL);

		sprintf(airportInfo, "Current airport: '%s', \"%s\"", id, name);
	}
	else {
		sprintf(airportInfo, "No airports were found!");
	}

	//XPLMDrawString(color, left + 5, top - 40, airportInfo, NULL, xplmFont_Basic);

	XPSetWidgetDescriptor(LocationText, airportInfo);
	XPLMDebugString(airportInfo);
}

void CreateWidgetWindow(int x, int y, int w, int h)
{
	int Item;

	int x2 = x + w;
	int y2 = y - h;
	char Buffer[255];

	sprintf(Buffer, "%s %s %s", "FlightJobs Connector", FlightJobsXVersionNumber, "- rhpa23");
	FlightJobsXWidget = XPCreateWidget(x, y, x2, y2,
		1,	// Visible
		Buffer,	// desc
		1,		// root
		NULL,	// no container
		xpWidgetClass_MainWindow);

	XPSetWidgetProperty(FlightJobsXWidget, xpProperty_MainWindowHasCloseBoxes, 1);

	FlightJobsXWindow = XPCreateWidget(x + 5, y - 20, x2 - 5, y2 + 230,
		1,	// Visible
		"",	// desc
		0,		// root
		FlightJobsXWidget,
		xpWidgetClass_SubWindow);

	XPSetWidgetProperty(FlightJobsXWindow, xpProperty_SubWindowType, xpSubWindowStyle_SubWindow);

	// Button region
	LoginXButton = XPCreateWidget(x + 90, y - 110, x + 190, y - 130,
		1, " Login ", 0, FlightJobsXWidget,
		xpWidgetClass_Button);

	XPSetWidgetProperty(LoginXButton, xpProperty_ButtonType, xpPushButton);
	//---
	RefreshXButton = XPCreateWidget(x + 90, y - 290, x + 190, y - 310,
		1, " Refresh ", 0, FlightJobsXWidget,
		xpWidgetClass_Button);

	XPSetWidgetProperty(RefreshXButton, xpProperty_ButtonType, xpPushButton);
	//---
	StartXButton = XPCreateWidget(x + 90, y - 330, x + 190, y - 350,
		1, " Start ", 0, FlightJobsXWidget,
		xpWidgetClass_Button);

	XPSetWidgetProperty(StartXButton, xpProperty_ButtonType, xpPushButton);

	

	// TextBox region
	UserNameCaption = XPCreateWidget(x + 20, y - 40, x + 160, y - 60,
		1,	// Visible
		"User name: ",// desc
		0,		// root
		FlightJobsXWidget,
		xpWidgetClass_Caption);
	//--
	UserNameTextBox = XPCreateWidget(x + 100, y - 40, x + 220, y - 60,
		1, "", 0, FlightJobsXWidget,
		xpWidgetClass_TextField);


	PassWordCaption = XPCreateWidget(x + 20, y - 70, x + 160, y - 90,
		1,	// Visible
		"Password: ",// desc
		0,		// root
		FlightJobsXWidget,
		xpWidgetClass_Caption);
	//--
	PassWordTextBox = XPCreateWidget(x + 100, y - 70, x + 220, y - 90,
		1, "", 0, FlightJobsXWidget,
		xpWidgetClass_TextField);

	MessageCaption = XPCreateWidget(x + 15, y - 275, x + 190, y - 295,
		1,	// Visible
		" ",// desc
		0,		// root
		FlightJobsXWidget,
		xpWidgetClass_Caption);

	XPSetWidgetProperty(UserNameTextBox, xpProperty_TextFieldType, xpTextEntryField);
	//XPSetWidgetProperty(PassWordTextBox, xpProperty_TextFieldType, xpTextEntryField);
	XPSetWidgetProperty(PassWordTextBox, xpProperty_PasswordMode, 1);
	XPSetWidgetProperty(PassWordTextBox, xpProperty_MaxCharacters, 20);
	XPSetWidgetProperty(UserNameTextBox, xpProperty_MaxCharacters, 20);

	LocationText = XPCreateWidget(x + 20, y - 160, x + 160, y - 180,
		1,	// Visible
		"Current airport:",// desc
		0,		// root
		FlightJobsXWidget,
		xpWidgetClass_Caption);

	XPAddWidgetCallback(FlightJobsXWidget, FlightJobsXHandler);
	XPSetWidgetProperty(RefreshXButton, xpProperty_Enabled, 0);
	XPSetWidgetProperty(StartXButton, xpProperty_Enabled, 0);
	XPSetWidgetProperty(LoginXButton, xpProperty_Enabled, 1);
}

int	FlightJobsXHandler(
	XPWidgetMessage			inMessage,
	XPWidgetID				inWidget,
	intptr_t				inParam1,
	intptr_t				inParam2)
{
	// When widget close cross is clicked we only hide the widget
	if (inMessage == xpMessage_CloseButtonPushed)
	{
		if (FlightJobsXMenuItem1 == 1)
		{
			//XPDestroyWidget(FlightJobsXWidget, 1);
			if (XPIsWidgetVisible(FlightJobsXWidget))
			{
				XPHideWidget(FlightJobsXWidget);
			}			
		}
		return 1;
	}

	// Process when a button on the widget is pressed
	if (inMessage == xpMsg_PushButtonPressed)
	{
		if (inParam1 == (intptr_t)LoginXButton)
		{
			if (login(1) == 1)
			{
				UpdateData();

				XPSetWidgetProperty(RefreshXButton, xpProperty_Enabled, 1);
				XPSetWidgetProperty(StartXButton, xpProperty_Enabled, 1);
				XPSetWidgetProperty(LoginXButton, xpProperty_Enabled, 0);
				XPSetWidgetProperty(UserNameTextBox, xpProperty_Enabled, 0);
				XPSetWidgetProperty(PassWordTextBox, xpProperty_Enabled, 0);
			}
			return 1;
		}
		if (inParam1 == (intptr_t)RefreshXButton)
		{
			XPSetWidgetDescriptor(MessageCaption, " ");
			UpdateData();
			return 1;
		}
		if (inParam1 == (intptr_t)StartXButton)
		{
			XPSetWidgetDescriptor(MessageCaption, " ");
			if (FlightJobsXIsStarted == 0)
			{
				XPSetWidgetDescriptor(StartXButton, " Finish ");
				FlightJobsXIsStarted = 1;
			}
			else
			{
				XPSetWidgetDescriptor(StartXButton, " Start ");
				FlightJobsXIsStarted = 0;
			}
			return 1;
		}
	}
	return 0;
}


// Process Menu 1 selections
void FlightJobsXMenuHandler(void * mRef, void * iRef)
{
	// Menu selected for widget
	if (!strcmp((char *)iRef, "FlightJobsX"))
	{
		if (FlightJobsXMenuItem1 == 0)
		{
			CreateWidgetWindow(60, 650, 280, 370);
			FlightJobsXMenuItem1 = 1;
		}
		else
		{
			XPShowWidget(FlightJobsXWidget);
		}
	}
}

// Process Menu 2 selections
void FlightJobsXMenuHandler2(void * mRef, void * iRef)
{
	
}


// ************************ Request region *************************

struct string {
	char *ptr;
	size_t len;
};

void init_string(struct string *s) {
	s->len = 0;
	s->ptr = malloc(s->len + 1);
	if (s->ptr == NULL) {
		fprintf(stderr, "malloc() failed\n");
		exit(EXIT_FAILURE);
	}
	s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
	size_t new_len = s->len + size*nmemb;
	s->ptr = realloc(s->ptr, new_len + 1);
	if (s->ptr == NULL) {
		fprintf(stderr, "realloc() failed\n");
		exit(EXIT_FAILURE);
	}
	memcpy(s->ptr + s->len, ptr, size*nmemb);
	s->ptr[new_len] = '\0';
	s->len = new_len;

	return size*nmemb;
}

int login(int num)
{
	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	if (curl) {
		struct string s;
		init_string(&s);

		curl_easy_setopt(curl, CURLOPT_URL, "localhost/api/WebApi/Login?id=1");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
		
		char userNameBuf[40];
		XPGetWidgetDescriptor(UserNameTextBox, userNameBuf, sizeof(userNameBuf));
		sprintf(userNameBuf, "Email: %s", userNameBuf);

		char passNameBuf[40];
		XPGetWidgetDescriptor(PassWordTextBox, passNameBuf, sizeof(passNameBuf));
		sprintf(passNameBuf, "Password: %s", passNameBuf);

		struct curl_slist *headers = NULL;
		headers = curl_slist_append(headers, userNameBuf);
		headers = curl_slist_append(headers, passNameBuf);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		// Do request 
		res = curl_easy_perform(curl);
		XPLMDebugString(s.ptr);

		/* Check for errors */
		if (res != CURLE_OK)
		{
			//fprintf(stderr, "Server connection failed: %s\n", curl_easy_strerror(res));
			XPSetWidgetDescriptor(MessageCaption, "Server connection failed");
			curl_easy_cleanup(curl);
			return 0;
		}
		else
		{
			long http_code = 0;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
			if (http_code == 200)
			{
				//fprintf(stderr, "Succeeded: ", s.ptr);
				XPSetWidgetDescriptor(MessageCaption, s.ptr);
			}
			else
			{
				//fprintf(stderr, "Failed: ", s.ptr);
				XPSetWidgetDescriptor(MessageCaption, s.ptr);
				curl_easy_cleanup(curl);
				return 0;
			}
		}
		
		//printf("%s\n", s.ptr);
		//free(s.ptr);

		/* always cleanup */
		curl_easy_cleanup(curl);

	}
	return 1;
}