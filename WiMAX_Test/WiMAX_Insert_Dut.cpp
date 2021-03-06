#include "stdafx.h"
#include "TestManager.h"
#include "WIMAX_Test.h"
#include "WIMAX_Test_Internal.h"
#include "IQmeasure.h"
#include "vDUT.h"
#include "IQlite_Timer.h"
#include "IQlite_Logger.h"

// This variable is declared in WIMAX_Test_Internal.cpp
extern vDUT_ID      g_WIMAX_Dut;
extern bool         g_ReloadDutDll;
extern TM_ID        g_WIMAX_Test_ID;
extern int          g_WIMAX_Test_timer;
extern const char  *g_vDUT_Version;

using namespace std;

// Input Parameter Container
map<string, WIMAX_SETTING_STRUCT> l_insertDutParamMap;

// Return Value Container 
map<string, WIMAX_SETTING_STRUCT> l_insertDutReturnMap;

struct tagParam
{
    char    DUT_DLL_FILENAME[MAX_BUFFER_SIZE];		/*!< Specify which DUT control DLL to be used. */
	char    CONNECTION_STRING[MAX_BUFFER_SIZE];		/*!< Specify the DUT connection string. */
    int     RELOAD_DUT_DLL;							/*!< Specify whether the DUT control DLL will be loaded into memory every time INSERT_DUT runs. Options are 1(reload), or 0(not reload) */
} l_insertDutParam;

struct tagReturn
{
	char    VDUT_VERSION[MAX_BUFFER_SIZE];			/*!< Information of the vDUT DLL, such as version. */
    char    DUT_VERSION[MAX_BUFFER_SIZE];			/*!< Information of the DUT control DLL, such as version. */
    char    ERROR_MESSAGE[MAX_BUFFER_SIZE];			
} l_insertDutReturn;

void ClearInsertDutReturn(void)
{
	l_insertDutParamMap.clear();
	l_insertDutReturnMap.clear();
}


//! Insert WIMAX DUT
/*!
 * Input Parameters
 *
 *  - Mandatory 
 *      -# A string that is used to determine which DUT register function will be used
 *
 * Return Values
 *      -# A string that contains all DUT info, such as FW version, driver version, chip revision, etc.
 *      -# A string for error message
 *
 * \return 0 No error occurred
 * \return -1 DUT failed to insert.  Please see the returned error message for details
 */
WIMAX_TEST_API int WIMAX_Insert_Dut(void)
{
    int  err = ERR_OK;
    int  dummyValue = 0;
	char vErrorMsg[MAX_BUFFER_SIZE]  = {'\0'};
	char logMessage[MAX_BUFFER_SIZE] = {'\0'};

    /*---------------------------------------*
     * Clear Return Parameters and Container *
     *---------------------------------------*/
	ClearReturnParameters(l_insertDutReturnMap);

    /*------------------------*
     * Respond to QUERY_INPUT *
     *------------------------*/
    err = TM_GetIntegerParameter(g_WIMAX_Test_ID, "QUERY_INPUT", &dummyValue);
    if( ERR_OK==err )
    {
        RespondToQueryInput(l_insertDutParamMap);
        return err;
    }
	else
	{
		// do nothing
        // Nothing to do
	}

    /*-------------------------*
     * Respond to QUERY_RETURN *
     *-------------------------*/
    err = TM_GetIntegerParameter(g_WIMAX_Test_ID, "QUERY_RETURN", &dummyValue);
    if( ERR_OK==err )
    {
        RespondToQueryReturn(l_insertDutReturnMap);
        return err;
    }
	else
	{
		// do nothing
	}

	try
	{
	   /*---------------------------------------*
		* g_WIMAX_Test_ID need to be valid (>=0) *
		*---------------------------------------*/
		if( g_WIMAX_Test_ID<0 )  
		{
			err = -1;
			LogReturnMessage(logMessage, MAX_BUFFER_SIZE, LOGGER_ERROR, "[WIMAX] WIMAX_Test_ID not valid.\n");
			throw logMessage;
		}
		else
		{
			LogReturnMessage(logMessage, MAX_BUFFER_SIZE, LOGGER_INFORMATION, "[WIMAX] WIMAX_Test_ID = %d.\n", g_WIMAX_Test_ID);
		}

		TM_ClearReturns(g_WIMAX_Test_ID);

		/*----------------------*
		 * Get input parameters *
		 *----------------------*/
		err = GetInputParameters(l_insertDutParamMap);
		if ( ERR_OK!=err )
		{
			LogReturnMessage(logMessage, MAX_BUFFER_SIZE, LOGGER_ERROR, "[WIMAX] Input parameters are not complete.\n");
			throw logMessage;
		}
		else
		{
			LogReturnMessage(logMessage, MAX_BUFFER_SIZE, LOGGER_INFORMATION, "[WIMAX] Get input parameters return OK.\n");
		}

		g_ReloadDutDll = l_insertDutParam.RELOAD_DUT_DLL==0 ? false : true;
		if( 0!=strlen(l_insertDutParam.DUT_DLL_FILENAME) ) 
		{
			// If DUT DLL filename is specified, load Dut DLL
			// If everytime needs to reload the DUT dll, or the DUT dll has never been loaded
			if( 0!=l_insertDutParam.RELOAD_DUT_DLL || -1==g_WIMAX_Dut )
			{
				err = ::vDUT_RegisterDutDll( "WIMAX", l_insertDutParam.DUT_DLL_FILENAME, &g_WIMAX_Dut );
				if ( ERR_OK!=err )
				{
					LogReturnMessage(logMessage, MAX_BUFFER_SIZE, LOGGER_ERROR, "[WIMAX] Fail to load Dut Dll.\n");
					throw logMessage;
				}
				else
				{
					LogReturnMessage(logMessage, MAX_BUFFER_SIZE, LOGGER_INFORMATION, "[WIMAX] vDUT_RegisterDutDll() load Dut Dll return OK.\n");
				}				
			}
			else
			{
				// do nothing
			}
		}
		else
		{			
			// No DUT DLL name is specified, return error
			// Log the error message
			err = -1;
			LogReturnMessage(logMessage, MAX_BUFFER_SIZE, LOGGER_ERROR, "[WIMAX] %s is not a valid DUT DLL name.\n", l_insertDutParam.DUT_DLL_FILENAME);	
			throw logMessage;
		}

		if(l_insertDutParam.CONNECTION_STRING!=NULL)
		{
			vDUT_AddStringParameter (g_WIMAX_Dut, "CONNECTION_STRING",	l_insertDutParam.CONNECTION_STRING);
		}
		else
		{
			// do nothing
		}		

		if( (0==err)&&(g_WIMAX_Dut>=0) )  // Status is OK
		{
			// INSERT_DUT
			err = vDUT_Run(g_WIMAX_Dut, "INSERT_DUT");			
			if ( ERR_OK!=err )
			{	// Check vDut return "ERROR_MESSAGE" or not, if "Yes", must handle it.
				err = ::vDUT_GetStringReturn(g_WIMAX_Dut, "ERROR_MESSAGE", vErrorMsg, MAX_BUFFER_SIZE);
				if ( ERR_OK==err )	// Get "ERROR_MESSAGE" from vDut
				{
					err = -1;	// set err to -1, means "Error".
					LogReturnMessage(logMessage, MAX_BUFFER_SIZE, LOGGER_ERROR, vErrorMsg);
					throw logMessage;
				}
				else	// Just return normal error message in this case
				{
					LogReturnMessage(logMessage, MAX_BUFFER_SIZE, LOGGER_ERROR, "[WIMAX] %s fail to run INSERT_DUT.\n", l_insertDutParam.DUT_DLL_FILENAME);
					throw logMessage;
				}
			}
			else
			{
				LogReturnMessage(logMessage, MAX_BUFFER_SIZE, LOGGER_INFORMATION, "[WIMAX] vDUT_Run(INSERT_DUT) return OK.\n");
			}


			// For retrieval of the vDUT version info
			err = ::vDUT_GetVersion(l_insertDutReturn.VDUT_VERSION, MAX_BUFFER_SIZE); 
			if ( ERR_OK!=err )
			{
				LogReturnMessage(logMessage, MAX_BUFFER_SIZE, LOGGER_ERROR, "[WIMAX] Fail to get vDut version information.\n");
				throw logMessage;
			}
			else
			{
				LogReturnMessage(logMessage, MAX_BUFFER_SIZE, LOGGER_INFORMATION, "[WIMAX] Get vDut version information return OK.\n");
			}

			// For retrieval of the DUT info
			err = ::vDUT_GetStringReturn(g_WIMAX_Dut, "DUT_VERSION", l_insertDutReturn.DUT_VERSION, MAX_BUFFER_SIZE);
			if ( ERR_OK!=err )
			{
				err = ERR_OK;	// This is an optional return parameter, thus always return OK
				sprintf_s(l_insertDutReturn.DUT_VERSION, MAX_BUFFER_SIZE, "No Return, Unknown Dut Version.\n");
			}
			else
			{
				// do nothing
			}
		}
		else
		{
			// Registration failed
			err = -1;
			LogReturnMessage(logMessage, MAX_BUFFER_SIZE, LOGGER_ERROR, "[WIMAX] %s failed to register WIMAX Technology.\n", l_insertDutParam.DUT_DLL_FILENAME);
		}

		/*----------------------*
		 *  Function Completed  *
		 *----------------------*/
		if (ERR_OK==err)
		{	
			sprintf_s(l_insertDutReturn.ERROR_MESSAGE, MAX_BUFFER_SIZE, "[Info] Function completed.");
		}
		else
		{
			// do nothing
		}
	}
	catch(char *msg)
    {
        ReturnErrorMessage(l_insertDutReturn.ERROR_MESSAGE, msg);
    }
    catch(...)
    {
		ReturnErrorMessage(l_insertDutReturn.ERROR_MESSAGE, "[WIMAX] Unknown Error!\n");
		err = -1;
    }

	/*-----------------------*
	 *  Return Test Results  *
	 *-----------------------*/
	InitializeInternalParameters();
	ReturnTestResults(l_insertDutReturnMap);


    return err;
}

void InitializeInsertDutContainers(void)
{
    /*------------------*
     * Input Parameters: *
     *------------------*/
    l_insertDutParamMap.clear();

    WIMAX_SETTING_STRUCT setting;

    setting.unit        = "";
    setting.helpText    = "Specify which DUT control DLL to be used\r\nIf no path with the specified DLL filename, the DLL will be loaded from the same folder where vDUT.DLL resides.";
    setting.type = WIMAX_SETTING_TYPE_STRING;
    strcpy_s(l_insertDutParam.DUT_DLL_FILENAME, MAX_BUFFER_SIZE, "TemplateDut.DLL");
    if (MAX_BUFFER_SIZE==sizeof(l_insertDutParam.DUT_DLL_FILENAME))    // Type_Checking
    {
        setting.value       = (void*)l_insertDutParam.DUT_DLL_FILENAME;
        l_insertDutParamMap.insert( pair<string,WIMAX_SETTING_STRUCT>("DUT_DLL_FILENAME", setting) );
    }
    else    
    {
        printf("Parameter Type Error!\n");
        exit(1);
    }

    setting.unit        = "";
	setting.helpText    = "Specify the connection string for the DUT";
	setting.type = WIMAX_SETTING_TYPE_STRING;
	strcpy_s(l_insertDutParam.CONNECTION_STRING, MAX_BUFFER_SIZE, "USB");
	if (MAX_BUFFER_SIZE==sizeof(l_insertDutParam.CONNECTION_STRING))    // Type_Checking
	{
		setting.value       = (void*)l_insertDutParam.CONNECTION_STRING;
		l_insertDutParamMap.insert( pair<string,WIMAX_SETTING_STRUCT>("CONNECTION_STRING", setting) );
	}
	else    
	{
		printf("Parameter Type Error!\n");
		exit(1);
	}

    setting.unit        = "";
    setting.helpText    = "Specify whether the DUT control DLL will be loaded into memory every time INSERT_DUT runs.\r\nOptions are 1(reload), or 0(not reload)\r\nIf set to 1(reload), REMOVE_DUT will unload the DLL from memory";
    setting.type = WIMAX_SETTING_TYPE_INTEGER;
    l_insertDutParam.RELOAD_DUT_DLL = 0;
    if (sizeof(int)==sizeof(l_insertDutParam.RELOAD_DUT_DLL))    // Type_Checking
    {
        setting.value       = (void*)&l_insertDutParam.RELOAD_DUT_DLL;
        l_insertDutParamMap.insert( pair<string,WIMAX_SETTING_STRUCT>("RELOAD_DUT_DLL", setting) );
    }
    else    
    {
        printf("Parameter Type Error!\n");
        exit(1);
    }

    
    /*----------------*
     * Return Values: *
     * ERROR_MESSAGE  *
     *----------------*/
    l_insertDutReturnMap.clear();

    l_insertDutReturn.VDUT_VERSION[0] = '\0';
    setting.type = WIMAX_SETTING_TYPE_STRING;
    if (MAX_BUFFER_SIZE==sizeof(l_insertDutReturn.VDUT_VERSION))    // Type_Checking
    {
        setting.value       = (void*)l_insertDutReturn.VDUT_VERSION;
        setting.unit        = "";
        setting.helpText    = "Information of the vDUT DLL, such as version.";
        l_insertDutReturnMap.insert( pair<string,WIMAX_SETTING_STRUCT>("VDUT_VERSION", setting) );
    }
    else    
    {
        printf("Parameter Type Error!\n");
        exit(1);
    }

    l_insertDutReturn.DUT_VERSION[0] = '\0';
    setting.type = WIMAX_SETTING_TYPE_STRING;
    if (MAX_BUFFER_SIZE==sizeof(l_insertDutReturn.DUT_VERSION))    // Type_Checking
    {
        setting.value       = (void*)l_insertDutReturn.DUT_VERSION;
        setting.unit        = "";
        setting.helpText    = "Information of the DUT control DLL, such as version.";
        l_insertDutReturnMap.insert( pair<string,WIMAX_SETTING_STRUCT>("DUT_VERSION", setting) );
    }
    else    
    {
        printf("Parameter Type Error!\n");
        exit(1);
    }

    l_insertDutReturn.ERROR_MESSAGE[0] = '\0';
    setting.type = WIMAX_SETTING_TYPE_STRING;
    if (MAX_BUFFER_SIZE==sizeof(l_insertDutReturn.ERROR_MESSAGE))    // Type_Checking
    {
        setting.value       = (void*)l_insertDutReturn.ERROR_MESSAGE;
        setting.unit        = "";
        setting.helpText    = "Error message occurred.";
        l_insertDutReturnMap.insert( pair<string,WIMAX_SETTING_STRUCT>("ERROR_MESSAGE", setting) );
    }
    else    
    {
        printf("Parameter Type Error!\n");
        exit(1);
    }

    return;
}
