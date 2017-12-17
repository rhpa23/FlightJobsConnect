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
#include "XPLMProcessing.h"
#include "XPLMDataAccess.h"
#include "XPLMUtilities.h"
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
int StartRequest();
int FinishRequest();
int LoginRequest();
void ReadSavedData();

FILE *	gOutputFile;
char	outputPath[255];
char	fileName[] = "/Resources/plugins/FlightJobs/LoginSavedData.ini";

XPWidgetID	FlightJobsXWidget, FlightJobsXWindow, FlightJobsXWindow2 = NULL;
XPWidgetID	RefreshXButton, LoginXButton, StartXButton = NULL;
XPWidgetID	LocationText, PayloadText, AircraftNumberCaption, AircraftDescCaption, AircraftFuelCaption = NULL;
XPWidgetID	MessageCaption, UserNameCaption, UserNameTextBox, PassWordCaption, PassWordTextBox = NULL;
XPWidgetID	HiddenUserIdCaption = NULL;

XPLMDataRef		ParkingBrake;

//char host[] = "localhost:5646";
char host[] = "flightjobs.gear.host";

char FlightJobsXVersionNumber[] = "v0.10";
char currentICAO[5];
char currentName[256];
char arrivalICAO[5];
char acfTailNumS[40];
char acfDescS[256];
float payloadF;
float fuelWeightF;

// Menu Prototypes
void FlightJobsXMenuHandler(void *, void *);
void FlightJobsXMenuHandler2(void *, void *);

int FlightJobsXMenuItem1;
int FlightJobsXIsStarted;
int RememberShown;
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

	strcpy(outName, "FlightJobs Connector");
	strcpy(outSig, "xplanesdk.examples.flightfobs");
	strcpy(outDesc, "Connect to FlightJobs system.");

	FlightJobsXMenuItem1 = 0;
	FlightJobsXIsStarted = 0;
	RememberShown = 0;
	// Create the menus
	FlightJobsXMenuItem = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "FlightJobs", NULL, 1);
	FlightJobsXMenuId = XPLMCreateMenu("FlightJobs", XPLMFindPluginsMenu(), FlightJobsXMenuItem, FlightJobsXMenuHandler, NULL);
	FlightJobsXMenuItem2 = XPLMAppendMenuItem(FlightJobsXMenuId, "Open connector", (void *)"Open connector", 1);

	ParkingBrake = XPLMFindDataRef("sim/flightmodel/controls/parkbrake");

	XPLMRegisterFlightLoopCallback(
		MyFlightLoopCallback,	/* Callback */
		5.0,					/* Interval */
		NULL);					/* refcon not used. */

	XPLMDebugString("FlightJobs info - Started \n");

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
	XPLMDestroyMenu(FlightJobsXMenuId);

	/* Unregister the callback */
	XPLMUnregisterFlightLoopCallback(MyFlightLoopCallback, NULL);
	
	/* Close the file */
	fclose(gOutputFile);

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
	/* The actual callback.  First we read the sim's time and the data. */
	//float	elapsed = XPLMGetElapsedTime();
	float	pBrake = XPLMGetDataf(ParkingBrake);
	
	if (FlightJobsXIsStarted == 1 && pBrake == 1.0)
	{

		SetCurrentICAO();

		if (RememberShown == 0 && strcmp(currentICAO, arrivalICAO) == 0)
		{
			if (FlightJobsXMenuItem1 == 0)
			{
				CreateWidgetWindow(60, 650, 320, 370);
				FlightJobsXMenuItem1 = 1;
			}
			else
			{
				XPShowWidget(FlightJobsXWidget);
			}
			RememberShown = 1;
		}
	}

	/* Return 1.0 to indicate that we want to be called again in 5 second. */
	return 1.0;
}

void UpdateData()
{
	// Aircraft payload
	payloadF = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/weight/m_fixed"));
	char payloadS[90];
	sprintf(payloadS, "Payload: %.2f %s", payloadF, "Kg");

	// Aircraft Number
	XPLMGetDatab(XPLMFindDataRef("sim/aircraft/view/acf_tailnum"), acfTailNumS, 0, 40);
	char aircraftNumberS[90];
	sprintf(aircraftNumberS, "Tail number: %s", acfTailNumS);

	// Description of the plane
	XPLMGetDatab(XPLMFindDataRef("sim/aircraft/view/acf_descrip"), acfDescS, 0, 256);
	char aircraftDescS[256];
	sprintf(aircraftDescS, "Aircraft: %s", acfDescS);

	char subAircraftDescS[45];
	memcpy(subAircraftDescS, &aircraftDescS[0], 44);
	subAircraftDescS[44] = '\0';
	
	// Aircraft FuelWeight
	fuelWeightF = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/weight/m_fuel_total"));
	char fuelWeightS[50];
	sprintf(fuelWeightS, "Fuel weight: %.2f %s", fuelWeightF, "Kg");

	char  airportInfo[256];

	if (SetCurrentICAO() == 1)
	{
		sprintf(airportInfo, "Current airport: '%s', \"%s\"", currentICAO, currentName);
	}
	else
	{
		sprintf(airportInfo, "No airports were found!");
	}

	XPSetWidgetDescriptor(PayloadText, payloadS);
	XPSetWidgetDescriptor(AircraftNumberCaption, aircraftNumberS);
	XPSetWidgetDescriptor(AircraftDescCaption, subAircraftDescS);
	XPSetWidgetDescriptor(AircraftFuelCaption, fuelWeightS);
	XPSetWidgetDescriptor(LocationText, airportInfo);
	//XPLMDebugString(airportInfo);
}

int SetCurrentICAO()
{
	/* First find the plane's position. */
	float lat = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/latitude"));
	float lon = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/longitude"));

	/* Find the nearest airport to us. */
	XPLMNavRef	ref = XPLMFindNavAid(NULL, NULL, &lat, &lon, NULL, xplm_Nav_Airport);
	if (ref != XPLM_NAV_NOT_FOUND)
	{
		/* If we found one, get its information, and say it. */
		XPLMGetNavAidInfo(ref, NULL, &lat, &lon, NULL, NULL, NULL, currentICAO, currentName, NULL);

		return 1;
	}

	return 0;
}

//CreateWidgetWindow(60, 650, 320, 370);
void CreateWidgetWindow(int x, int y, int w, int h)
{
	int Item;

	int x2 = x + w;
	int y2 = y - h;//280
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

	FlightJobsXWindow2 = XPCreateWidget(x + 5, y - 145, x2 - 5, y2 + 5,
		1,	// Visible
		"",	// desc
		0,		// root
		FlightJobsXWidget,
		xpWidgetClass_SubWindow);

	XPSetWidgetProperty(FlightJobsXWindow, xpProperty_SubWindowType, xpSubWindowStyle_SubWindow);
	XPSetWidgetProperty(FlightJobsXWindow2, xpProperty_SubWindowType, xpSubWindowStyle_SubWindow);

	// Button region
	LoginXButton = XPCreateWidget(x + 110, y - 110, x + 210, y - 130, 1, " Login ", 0, FlightJobsXWidget, xpWidgetClass_Button);
	XPSetWidgetProperty(LoginXButton, xpProperty_ButtonType, xpPushButton);

	RefreshXButton = XPCreateWidget(x + 110, y - 300, x + 210, y - 320, 1, " Refresh ", 0, FlightJobsXWidget, xpWidgetClass_Button);
	XPSetWidgetProperty(RefreshXButton, xpProperty_ButtonType, xpPushButton);

	StartXButton = XPCreateWidget(x + 110, y - 330, x + 210, y - 350, 1, " Start ", 0, FlightJobsXWidget, xpWidgetClass_Button);
	XPSetWidgetProperty(StartXButton, xpProperty_ButtonType, xpPushButton);

	// TextBox region
	UserNameCaption = XPCreateWidget(x + 20, y - 40, x + 160, y - 60, 1, "User name: ", 0, FlightJobsXWidget, xpWidgetClass_Caption);
	UserNameTextBox = XPCreateWidget(x + 100, y - 40, x + 280, y - 60, 1, "", 0, FlightJobsXWidget,	xpWidgetClass_TextField);

	PassWordCaption = XPCreateWidget(x + 20, y - 70, x + 160, y - 90, 1, "Password: ", 0, FlightJobsXWidget, xpWidgetClass_Caption);
	PassWordTextBox = XPCreateWidget(x + 100, y - 70, x + 280, y - 90, 1, "", 0, FlightJobsXWidget,	xpWidgetClass_TextField);

	XPSetWidgetProperty(UserNameTextBox, xpProperty_TextFieldType, xpTextEntryField);
	XPSetWidgetProperty(PassWordTextBox, xpProperty_PasswordMode, 1);
	XPSetWidgetProperty(PassWordTextBox, xpProperty_MaxCharacters, 20);
	XPSetWidgetProperty(UserNameTextBox, xpProperty_MaxCharacters, 20);

	// Caption Region 
	LocationText = XPCreateWidget(x + 10, y - 160, x + 220, y - 180, 1,	" ",	0, FlightJobsXWidget, xpWidgetClass_Caption);
	PayloadText = XPCreateWidget(x + 10, y - 180, x + 220, y - 200, 1, " ", 0, FlightJobsXWidget, xpWidgetClass_Caption);
	AircraftNumberCaption = XPCreateWidget(x + 10, y - 200, x + 220, y - 220, 1, " ", 0, FlightJobsXWidget, xpWidgetClass_Caption);
	AircraftDescCaption = XPCreateWidget(x + 10, y - 220, x + 220, y - 240, 1, " ", 0, FlightJobsXWidget, xpWidgetClass_Caption);
	AircraftFuelCaption = XPCreateWidget(x + 10, y - 240, x + 220, y - 260, 1, " ", 0, FlightJobsXWidget, xpWidgetClass_Caption);

	MessageCaption = XPCreateWidget(x + 15, y - 275, x + 220, y - 295, 1, " ", 0, FlightJobsXWidget, xpWidgetClass_Caption);
	HiddenUserIdCaption = XPCreateWidget(x, y, x, y, 0, " ", 0, FlightJobsXWidget, xpWidgetClass_Caption);

	XPAddWidgetCallback(FlightJobsXWidget, FlightJobsXHandler);
	XPSetWidgetProperty(RefreshXButton, xpProperty_Enabled, 0);
	XPSetWidgetProperty(StartXButton, xpProperty_Enabled, 0);
	XPSetWidgetProperty(LoginXButton, xpProperty_Enabled, 1);

	ReadSavedData();
}

void ReadSavedData() {
	XPLMGetSystemPath(outputPath);
	strcat(outputPath, fileName);

	gOutputFile = fopen(outputPath, "r");
	if (gOutputFile) {
		char line[512];
		char *search = "|";
		char *userNameBuff;
		char *passWordBuff;
		while (!feof(gOutputFile)) {
			if (fgets(line, sizeof(line), gOutputFile)) {

				userNameBuff = strtok(line, search);
				passWordBuff = strtok(NULL, search);

				XPSetWidgetDescriptor(UserNameTextBox, userNameBuff);
				XPSetWidgetDescriptor(PassWordTextBox, passWordBuff);
			}
		}
	}
	fclose(gOutputFile);
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
			if (LoginRequest() == 1)
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
			UpdateData();

			if (FlightJobsXIsStarted == 0)
			{
				if (StartRequest() == 1)
				{
					
					XPSetWidgetDescriptor(StartXButton, " Finish ");
					FlightJobsXIsStarted = 1;
					//XPSetWidgetDescriptor(MessageCaption, "Job approved and started at: ");
				}
				else
				{
					XPLMDebugString("\n FlightJobs start erro.");
				}
			}
			else
			{
				if (FinishRequest() == 1)
				{
					XPSetWidgetDescriptor(StartXButton, " Start ");
					FlightJobsXIsStarted = 0;
				}
				else
				{
					XPLMDebugString("\n FlightJobs finish erro.");
				}
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
	if (!strcmp((char *)iRef, "Open connector"))
	{
		if (FlightJobsXMenuItem1 == 0)
		{
			CreateWidgetWindow(60, 650, 320, 370);
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

int LoginRequest() 
{
	CURL *curl;
	CURLcode res;
	int result = 0;

	curl = curl_easy_init();
	if (curl) {
		struct string s;
		init_string(&s);
		
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
		
		struct curl_slist *headers = NULL;
		
		char endPointBuf[256];
		sprintf(endPointBuf, "%s/api/AuthenticationApi/Login", host);

		char userNameBuffer[250];
		curl_easy_setopt(curl, CURLOPT_URL, endPointBuf);
		XPGetWidgetDescriptor(UserNameTextBox, userNameBuffer, sizeof(userNameBuffer));
		char userNameBuf[250];
		sprintf(userNameBuf, "Email: %s", userNameBuffer);

		char pwBuffer[250];
		char passNameBuf[240];
		XPGetWidgetDescriptor(PassWordTextBox, pwBuffer, sizeof(pwBuffer));
		sprintf(passNameBuf, "Password: %s", pwBuffer);
			
		headers = curl_slist_append(headers, userNameBuf);
		headers = curl_slist_append(headers, passNameBuf);
		
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		// Do request 
		res = curl_easy_perform(curl);

		/* Check for errors */
		if (res != CURLE_OK)
		{
			XPSetWidgetDescriptor(MessageCaption, "Server connection failed");
		}
		else
		{
			long http_code = 0;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
			if (http_code == 200)
			{
				//XPSetWidgetDescriptor(MessageCaption, s.ptr);
				XPSetWidgetDescriptor(HiddenUserIdCaption, s.ptr);
				result = 1;

				/* Write the data to a file. */
				XPLMGetSystemPath(outputPath);
				strcat(outputPath, fileName);

				gOutputFile = fopen(outputPath, "w");

				/* Write a file. */
				fprintf(gOutputFile, "%s|%s", userNameBuffer, pwBuffer);

				fflush(gOutputFile);
			}
			else
			{
				XPSetWidgetDescriptor(MessageCaption, s.ptr);
			}
		}
		
		free(s.ptr);

		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	return result;
}


int StartRequest()
{
	CURL *curl;
	CURLcode res;
	int result = 0;

	curl = curl_easy_init();
	if (curl) {
		struct string sWriteData;
		init_string(&sWriteData);

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sWriteData);

		struct curl_slist *headers = NULL;

		char endPointBuf[256];
		sprintf(endPointBuf, "%s/api/JobApi/StartJob", host);

		curl_easy_setopt(curl, CURLOPT_URL, endPointBuf);
		char icaoBuf[50];
		sprintf(icaoBuf, "ICAO: %s", currentICAO);

		char payloadBuf[40];
		sprintf(payloadBuf, "Payload: %f", payloadF);

		char idTempBuffer[150];
		XPGetWidgetDescriptor(HiddenUserIdCaption, idTempBuffer, sizeof(idTempBuffer));
		char userIdBuf[140];
		sprintf(userIdBuf, "UserId: %s", idTempBuffer);

		char fuelWeightBuf[50];
		sprintf(fuelWeightBuf, "FuelWeight: %f", fuelWeightF);

		headers = curl_slist_append(headers, fuelWeightBuf);
		headers = curl_slist_append(headers, userIdBuf);
		headers = curl_slist_append(headers, icaoBuf);
		headers = curl_slist_append(headers, payloadBuf);

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		// Do request 
		res = curl_easy_perform(curl);
		//XPLMDebugString(s.ptr);

		/* Check for errors */
		if (res != CURLE_OK)
		{
			XPSetWidgetDescriptor(MessageCaption, "Server connection failed");
		}
		else
		{
			long http_code = 0;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
			if (http_code == 200)
			{
				

				XPSetWidgetDescriptor(MessageCaption, sWriteData.ptr);

				char line[256];
				strcpy(line, sWriteData.ptr);

				memcpy(arrivalICAO, &line[8], 4);
				arrivalICAO[4] = '\0';

				char arrivalICAOInfo[256];
				sprintf(arrivalICAOInfo, "FlightJobs - arrival ICAO is: %s \n", arrivalICAO);
				XPLMDebugString(arrivalICAOInfo);

				result = 1;
			}
			else
			{
				XPSetWidgetDescriptor(MessageCaption, sWriteData.ptr);
			}
		}

		free(sWriteData.ptr);
		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	return result;
}

int FinishRequest()
{

	//if (strcmp(currentICAO, arrivalICAO) == 0)
	//{
	//	char testInfo[256];
	//	sprintf(testInfo, "FlightJobs - SAME: current = %s  arrival = %s \n", currentICAO, arrivalICAO);
	//	XPLMDebugString(testInfo);
	//}
	//else
	//{
	//	char testInfo[256];
	//	sprintf(testInfo, "FlightJobs - current = %s  arrival = %s \n", currentICAO, arrivalICAO);
	//	XPLMDebugString(testInfo);
	//}

	CURL *curl;
	CURLcode res;
	int result = 0;

	curl = curl_easy_init();
	if (curl) {
		struct string sWriteData;
		init_string(&sWriteData);

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sWriteData);

		struct curl_slist *headers = NULL;

		char endPointBuf[256];
		sprintf(endPointBuf, "%s/api/JobApi/FinishJob", host);

		curl_easy_setopt(curl, CURLOPT_URL, endPointBuf);
		char icaoBuf[50];
		sprintf(icaoBuf, "ICAO: %s", currentICAO);

		char payloadBuf[40];
		sprintf(payloadBuf, "Payload: %f", payloadF);

		char idTempBuffer[150];
		XPGetWidgetDescriptor(HiddenUserIdCaption, idTempBuffer, sizeof(idTempBuffer));
		char userIdBuf[140];
		sprintf(userIdBuf, "UserId: %s", idTempBuffer);

		char tailNumBuf[60];
		sprintf(tailNumBuf, "TailNumber: %s", acfTailNumS);

		char acfDescBuf[256];
		sprintf(acfDescBuf, "PlaneDescription: %s", acfDescS);

		char fuelWeightBuf[50];
		sprintf(fuelWeightBuf, "FuelWeight: %f", fuelWeightF);

		headers = curl_slist_append(headers, fuelWeightBuf);
		headers = curl_slist_append(headers, userIdBuf);
		headers = curl_slist_append(headers, icaoBuf);
		headers = curl_slist_append(headers, payloadBuf);
		headers = curl_slist_append(headers, tailNumBuf);
		headers = curl_slist_append(headers, acfDescBuf);

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		// Do request 
		res = curl_easy_perform(curl);
		//XPLMDebugString(s.ptr);

		/* Check for errors */
		if (res != CURLE_OK)
		{
			XPSetWidgetDescriptor(MessageCaption, "Server connection failed");
		}
		else
		{
			long http_code = 0;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
			if (http_code == 200)
			{
				XPSetWidgetDescriptor(MessageCaption, sWriteData.ptr);
				result = 1;
			}
			else
			{
				XPSetWidgetDescriptor(MessageCaption, sWriteData.ptr);
			}
		}

		free(sWriteData.ptr);
		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	return result;
}