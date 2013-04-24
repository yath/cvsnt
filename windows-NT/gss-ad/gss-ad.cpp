/*
	GSSAPI wrapper for Active Directory
    Copyright (C) 2003-5 Tony Hoyle and March-Hare Software Ltd

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License version 2.1 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "stdafx.h"

OM_uint32 gss_import_name (
	    OM_uint32 *minor_status,
		gss_buffer_t input_name_buffer,
		gss_OID input_name_type,
		gss_name_t* output_name)
{
	LPSTR szClass = (char*)malloc(input_name_buffer->length+1);
	LPSTR szName;
	DWORD dwLength;
	LPSTR szSpn;

	memcpy(szClass,input_name_buffer->value,input_name_buffer->length);
	szClass[input_name_buffer->length]='\0';

	szName=strchr(szClass,'@');
	if(!szName)
	{
		free(szClass);
		return GSS_S_ERROR;
	}
	*(szName++)='\0';
	strlwr(szName);

	szSpn = (LPSTR)malloc(1024);
	dwLength=1024;
	if(DsMakeSpnA(szClass,szName,NULL,0,NULL,&dwLength,szSpn))
	{
		free(szClass);
		free(szSpn);
		return GSS_S_ERROR;
	}
	free(szClass);

	*output_name = szSpn;
	*minor_status = 0;
	return 0;
}

OM_uint32  gss_init_sec_context (
        OM_uint32 *minor_status,
        gss_cred_id_t claimant_cred_handle,
        gss_ctx_id_t *context_handle,
        gss_name_t target_name,
        gss_OID mech_type,
        int req_flags,
        int time_req,
        gss_channel_bindings_t input_channel_bindings,
        gss_buffer_t input_token,
        gss_OID *actual_mech_types,
        gss_buffer_t output_token,
        int *ret_flags,
        OM_int32 *time_rec)
{
	gss_cred_id_t credhandle={0};
	gss_ctx_id_t *pctx;

	SecBuffer InputBuf[1] = {input_token?input_token->length:0,SECBUFFER_TOKEN,input_token?input_token->value:0};
	SecBuffer OutputBuf[1] = {4096,SECBUFFER_TOKEN,malloc(4096)};
	SecBufferDesc InputBuffer[1] = {SECBUFFER_VERSION, 1, InputBuf};
	SecBufferDesc OutputBuffer[1] = {SECBUFFER_VERSION, 1, OutputBuf};
	OM_uint32 ret;
	TimeStamp tr;
	unsigned long rf;

	if(claimant_cred_handle.dwLower==0 && claimant_cred_handle.dwUpper==0)
	{
		static gss_cred_id_t global_client_cred={0};
		if(global_client_cred.dwLower==0 || global_client_cred.dwUpper==0)
		{
			ret = AcquireCredentialsHandleA(NULL,"Kerberos",SECPKG_CRED_OUTBOUND,NULL,NULL,NULL,NULL,&global_client_cred,NULL);
			if(ret)
				return ret;
		}
		credhandle = global_client_cred;
	}
	else
		credhandle = claimant_cred_handle;

	if(context_handle->dwLower==0 && context_handle->dwUpper==0)
		pctx = NULL;
	else
		pctx = context_handle;

	ret = InitializeSecurityContextA(
		&credhandle, pctx, target_name, req_flags, 0, SECURITY_NETWORK_DREP,
		input_token?InputBuffer:NULL,0, pctx?NULL:context_handle, OutputBuffer, &rf, &tr); 
	output_token->length = OutputBuf[0].cbBuffer;
	output_token->value = OutputBuf[0].pvBuffer;

	*minor_status = 0;
	if(time_rec)
		*time_rec=tr.LowPart;
	if(ret_flags)
		*ret_flags=rf;
	return ret;
}

OM_uint32  gss_acquire_cred (
        OM_uint32 *minor_status,
        gss_name_t desired_name,
        OM_uint32 time_req,
        gss_OID_set desired_mechs,
        int cred_usage,
        gss_cred_id_t *output_cred_handle,
        gss_OID_set *actual_mechs,
        OM_int32 *time_rec)
{
	*minor_status = 0;

	OM_uint32 ret = AcquireCredentialsHandleA(desired_name,"Kerberos",cred_usage,NULL,NULL,NULL,NULL,output_cred_handle,(TimeStamp*)time_rec);
	return ret;
}


OM_uint32  gss_release_name (
        OM_uint32 *minor_status,
        gss_name_t *name)
{
	free(*name);
	*name=NULL;
	*minor_status = 0;
	return 0;
}

OM_uint32  gss_accept_sec_context (
        OM_uint32 *minor_status,
        gss_ctx_id_t *context_handle,
        gss_cred_id_t verifier_cred_handle,
        gss_buffer_t input_token,
        gss_channel_bindings_t input_chan_bindings,
        gss_name_t *src_name,
        gss_OID *actual_mech_type,
        gss_buffer_t output_token,
        OM_uint32 *ret_flags,
        OM_uint32 *time_rec,
        gss_cred_id_t *delegated_cred_handle)
{
	gss_cred_id_t credhandle={0};
	gss_ctx_id_t *pctx;

	SecBuffer InputBuf[1] = {input_token->length,SECBUFFER_TOKEN,input_token->value};
	SecBuffer OutputBuf[1] = {4096,SECBUFFER_TOKEN,malloc(4096)};
	SecBufferDesc InputBuffer[1] = {SECBUFFER_VERSION, 1, InputBuf};
	SecBufferDesc OutputBuffer[1] = {SECBUFFER_VERSION, 1, OutputBuf};
	OM_uint32 ret;
	TimeStamp tr;
	unsigned long rf;

	if(context_handle->dwLower==0 && context_handle->dwUpper==0)
		pctx = NULL;
	else
		pctx = context_handle;

	ret = AcceptSecurityContext(&verifier_cred_handle, pctx, InputBuffer, ASC_REQ_MUTUAL_AUTH,
				SECURITY_NETWORK_DREP, context_handle, OutputBuffer, &rf, &tr);
	output_token->length = OutputBuf[0].cbBuffer;
	output_token->value = OutputBuf[0].pvBuffer;

	if(!ret)
	{
		SecPkgContext_NamesA names = {0};
		QueryContextAttributes(context_handle, SECPKG_ATTR_NAMES, &names);
		*src_name = strdup(names.sUserName);
		FreeContextBuffer(&names);
	}

	*minor_status = 0;
	if(time_rec)
		*time_rec=tr.LowPart;
	if(ret_flags)
		*ret_flags=rf;
	return ret;
}


OM_uint32 gss_display_name (
		OM_uint32 *minor_status,
		gss_name_t input_name,
		gss_buffer_t output_name_buffer,
		gss_OID *output_name_type)
{
	char *p = strchr(input_name,'\\');
	if(!p) p=input_name;
	// For now we just assume the name is a DOMAIN\user returned from QueryContextAttributes, above.
	// This could be handled better...
	output_name_buffer->value=strdup(p);
	output_name_buffer->length=strlen(input_name);
	*output_name_type=GSS_C_NT_HOSTBASED_SERVICE;
	return GSS_S_COMPLETE;
}


OM_uint32 gss_display_status (
        OM_uint32 *minor_status,
        int status_value,
        int status_type,
        gss_OID mech_type,
        OM_uint32 *message_context,
        gss_buffer_t status_string)
{
	*minor_status = 0;
	 status_string->value=malloc(2048);
	 sprintf((char*)status_string->value,"Unknown error %08x",status_value);

     FormatMessageA(
		FORMAT_MESSAGE_IGNORE_INSERTS
	    | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
	  status_value,
	  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	  (char*)status_string->value,
	  2048,
	  NULL);
	status_string->length=strlen((char*)status_string->value);
	return 0;
}


OM_uint32 gss_unwrap (
		OM_uint32 * minor_status,
		gss_ctx_id_t context_handle,
		gss_buffer_t input_message_buffer,
		gss_buffer_t output_message_buffer,
		int * conf_state,
		gss_qop_t * qop_state)
{
	SecPkgContext_Sizes sizes={0};
	BOOL b = QueryContextAttributes(&context_handle,SECPKG_ATTR_SIZES,&sizes);
	SecBuffer InputBuffer[2] = { { input_message_buffer->length,SECBUFFER_STREAM,input_message_buffer->value },
								 { 0,SECBUFFER_DATA,NULL },
							   };
	SecBufferDesc InputBufferDesc = {SECBUFFER_VERSION, 2, InputBuffer};
	OM_uint32 ret;
	ret = DecryptMessage(&context_handle, &InputBufferDesc, 0, (unsigned long*)qop_state);
	output_message_buffer->value=InputBuffer[1].pvBuffer;
	output_message_buffer->length=InputBuffer[1].cbBuffer;
	*minor_status = 0;
	return ret;
}


OM_uint32 gss_wrap (
		OM_uint32 *minor_status,
		gss_ctx_id_t context_handle,
		int conf_req_flag,
		gss_qop_t qop_req,
		gss_buffer_t input_message_buffer,
		int *conf_state,
		gss_buffer_t output_message_buffer)
{
	*minor_status = 0;
	SecPkgContext_Sizes sizes={0};
	BOOL b = QueryContextAttributes(&context_handle,SECPKG_ATTR_SIZES,&sizes);
	char *buffer = (char*)malloc(sizes.cbSecurityTrailer+input_message_buffer->length+sizes.cbBlockSize);
	SecBuffer InputBuffer[3] = { { sizes.cbSecurityTrailer,SECBUFFER_TOKEN,buffer },
								 { input_message_buffer->length,SECBUFFER_DATA,buffer+sizes.cbSecurityTrailer },
							     { sizes.cbBlockSize,SECBUFFER_PADDING,buffer+sizes.cbSecurityTrailer+input_message_buffer->length }
							   };
	SecBufferDesc InputBufferDesc = {SECBUFFER_VERSION, 3, InputBuffer};
	OM_uint32 ret;
	memcpy(InputBuffer[1].pvBuffer,input_message_buffer->value,input_message_buffer->length);
	ret = EncryptMessage(&context_handle, conf_req_flag? 0:KERB_WRAP_NO_ENCRYPT, &InputBufferDesc, 0);
	output_message_buffer->value=buffer;
	memmove(buffer+InputBuffer[0].cbBuffer,InputBuffer[1].pvBuffer,InputBuffer[1].cbBuffer);
	memmove(buffer+InputBuffer[0].cbBuffer+InputBuffer[1].cbBuffer,InputBuffer[2].pvBuffer,InputBuffer[2].cbBuffer);
	output_message_buffer->length=InputBuffer[0].cbBuffer+InputBuffer[1].cbBuffer+InputBuffer[2].cbBuffer;
	return ret;
}


OM_uint32 gss_release_buffer (
        OM_uint32 *minor_status,
        gss_buffer_t buffer)
{
	*minor_status = 0;
	free(buffer->value);
	buffer->length=0;
	buffer->value=NULL;
	return 0;
}
