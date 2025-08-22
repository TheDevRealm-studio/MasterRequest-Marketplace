/*
==========================================================================================
File: MasterHttpRequestBPLibrary.cpp
Author: Mario Tarosso
Year: 2023
Publisher: MJGT Studio
==========================================================================================
*/
#include "MasterHttpRequestBPLibrary.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Async/Async.h"
#include "Dom/JsonObject.h"

namespace
{
void SaveDebugToFile(const FString& url, E_RequestType_CPP httpMethod, TSharedPtr<FJsonObject> JsonObject, FHttpResponsePtr Response, const FResponseData& ReturnData)
{
    FString ResponseInfo;
    ResponseInfo += "\n\nServer Header:\n";
    for (auto& Header : Response->GetAllHeaders())
    {
        ResponseInfo += Header + "\n";
    }

    ResponseInfo += "\n\nServer Payload:\n" + Response->GetContentAsString();
    ResponseInfo += "\n\nServer Response:\n" + ReturnData.Data;
    ResponseInfo += "\n\nStatus code:\n" + FString::FromInt(ReturnData.StatusCode);

    if (JsonObject.IsValid() && JsonObject->Values.Num() > 0)
    {
        ResponseInfo += "\n\n--/--\n\n";
        ResponseInfo += "Client Payload:\n";
        for (auto& Field : JsonObject->Values)
        {
            ResponseInfo += Field.Key + ": " + Field.Value->AsString() + "\n";
        }
        ResponseInfo += "\n";
    }

    FString Timestamp = FDateTime::Now().ToString(TEXT("%d-%m-%Y_%H-%M-%S"));
    FString SlugifiedURL = url;
    SlugifiedURL.ReplaceInline(TEXT("//"), TEXT(" "));
    SlugifiedURL.ReplaceInline(TEXT("/"), TEXT(" "));
    SlugifiedURL.ReplaceInline(TEXT(":"), TEXT("_"));
    SlugifiedURL.ReplaceInline(TEXT("http"), TEXT("http_"));
    SlugifiedURL.ReplaceInline(TEXT("https"), TEXT("https_"));

    FString FileName = SlugifiedURL + "-" + StaticEnum<E_RequestType_CPP>()->GetDisplayNameTextByValue(static_cast<int64>(httpMethod)).ToString() + "-" + Timestamp + ".txt";
    FString FileDirectory = FPaths::ProjectSavedDir() + "/DebugHTTP";
    FString FullPath = FileDirectory + "/" + FileName;

    if (!FPaths::DirectoryExists(FileDirectory))
    {
        FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*FileDirectory);
    }

    if (FFileHelper::SaveStringToFile(ResponseInfo, *FullPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Saved response information to %s"), *FullPath);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save response information to %s"), *FullPath);
    }
}

void ProcessRequestInternal(FString url, E_RequestType_CPP httpMethod, const TArray<FKeyValuePair>& bodyPayload, const TArray<FKeyValuePair>& headers, FRequestReturn callback, bool debugResponse, bool bEncodePayload)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();

    HttpRequest->SetURL(url);
    HttpRequest->SetVerb(StaticEnum<E_RequestType_CPP>()->GetDisplayNameTextByValue(static_cast<int64>(httpMethod)).ToString());

    for (const auto& KeyValuePair : headers)
    {
        FString Value = bEncodePayload ? FGenericPlatformHttp::UrlEncode(KeyValuePair.Value) : KeyValuePair.Value;
        HttpRequest->SetHeader(KeyValuePair.Key, Value);
    }

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    for (const auto& KeyValuePair : bodyPayload)
    {
        if (!KeyValuePair.Key.IsEmpty() && !KeyValuePair.Value.IsEmpty())
        {
            FString Value = bEncodePayload ? FGenericPlatformHttp::UrlEncode(KeyValuePair.Value) : KeyValuePair.Value;
            JsonObject->SetStringField(KeyValuePair.Key, Value);
        }
    }

    FString BodyData;
    if (JsonObject.IsValid() && JsonObject->Values.Num() > 0)
    {
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyData);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    }

    HttpRequest->SetContentAsString(BodyData);
    if (HttpRequest->GetHeader(TEXT("Content-Type")).IsEmpty())
    {
        HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    }

    HttpRequest->OnProcessRequestComplete().BindLambda([callback, debugResponse, url, httpMethod, JsonObject](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
    {
        FResponseData ReturnData;
        ReturnData.bSuccess = bWasSuccessful;
        if (Response.IsValid())
        {
            ReturnData.Data = Response->GetContentAsString();
            ReturnData.StatusCode = Response->GetResponseCode();
            ReturnData.ErrorMessage = Response->GetContentAsString();

            if (debugResponse)
            {
                SaveDebugToFile(url, httpMethod, JsonObject, Response, ReturnData);
            }
        }
        else
        {
            ReturnData.ErrorMessage = "No valid response.";
            ReturnData.StatusCode = 0;
        }

        callback.ExecuteIfBound(ReturnData);
    });

    HttpRequest->ProcessRequest();
}
}

FKeyValuePair UMasterHttpRequestBPLibrary::MakeHeader(EHttpHeaderField Header, FString Value, FString CustomName)
{
    FKeyValuePair Pair;
    switch (Header)
    {
        case EHttpHeaderField::ContentType:
            Pair.Key = TEXT("Content-Type");
            break;
        case EHttpHeaderField::Accept:
            Pair.Key = TEXT("Accept");
            break;
        case EHttpHeaderField::Authorization:
            Pair.Key = TEXT("Authorization");
            break;
        case EHttpHeaderField::Custom:
        default:
            Pair.Key = CustomName;
            break;
    }
    Pair.Value = Value;
    return Pair;
}

void UMasterHttpRequestBPLibrary::MasterRequestAdvanced(FString url, E_RequestType_CPP httpMethod, TArray<FKeyValuePair> bodyPayload, TArray<FKeyValuePair> headers, FRequestReturn callback, bool bEncodePayload, bool bAsync, bool debugResponse)
{
    if (bAsync)
    {
        Async(EAsyncExecution::TaskGraph, [=]()
        {
            ProcessRequestInternal(url, httpMethod, bodyPayload, headers, callback, debugResponse, bEncodePayload);
        });
    }
    else
    {
        ProcessRequestInternal(url, httpMethod, bodyPayload, headers, callback, debugResponse, bEncodePayload);
    }
}

void UMasterHttpRequestBPLibrary::MasterRequest(FString url, E_RequestType_CPP httpMethod, FRequestReturn callback)
{
    MasterRequestAdvanced(url, httpMethod, TArray<FKeyValuePair>(), TArray<FKeyValuePair>(), callback, true, false, false);
}

void UMasterHttpRequestBPLibrary::MasterRequestWithPayloadAndHeaders(FString url, E_RequestType_CPP httpMethod, TArray<FKeyValuePair> bodyPayload, TArray<FKeyValuePair> headers, FRequestReturn callback, bool debugResponse)
{
    MasterRequestAdvanced(url, httpMethod, bodyPayload, headers, callback, true, false, debugResponse);
}

void UMasterHttpRequestBPLibrary::MasterRequestWithPayloadAndHeadersWithoutEncoding(FString url, E_RequestType_CPP httpMethod, TArray<FKeyValuePair> bodyPayload, TArray<FKeyValuePair> headers, FRequestReturn callback, bool debugResponse)
{
    MasterRequestAdvanced(url, httpMethod, bodyPayload, headers, callback, false, false, debugResponse);
}

void UMasterHttpRequestBPLibrary::MasterRequestAsync(FString url, E_RequestType_CPP httpMethod, TArray<FKeyValuePair> bodyPayload, TArray<FKeyValuePair> headers, FRequestReturn callback, bool debugResponse)
{
    MasterRequestAdvanced(url, httpMethod, bodyPayload, headers, callback, true, true, debugResponse);
}

void UMasterHttpRequestBPLibrary::MasterRequestAsyncWithoutEncoding(FString url, E_RequestType_CPP httpMethod, TArray<FKeyValuePair> bodyPayload, TArray<FKeyValuePair> headers, FRequestReturn callback, bool debugResponse)
{
    MasterRequestAdvanced(url, httpMethod, bodyPayload, headers, callback, false, true, debugResponse);
}

void UMasterHttpRequestBPLibrary::DecodeJson(FString jsonString, FString key, bool& success, FString& value)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(jsonString);
    success = FJsonSerializer::Deserialize(Reader, JsonObject);

    if (success && JsonObject.IsValid())
    {
        TArray<FString> keys;
        key.ParseIntoArray(keys, TEXT("."), true);

        for (auto k : keys)
        {
            if (JsonObject->HasField(k))
            {
                if (JsonObject->TryGetStringField(k, value))
                {
                    success = true;
                    return;
                }
                else
                {
                    JsonObject = JsonObject->GetObjectField(k);
                }
            }
            else
            {
                success = false;
                value = TEXT("");
                return;
            }
        }

        success = JsonObject->TryGetStringField(keys.Last(), value);
    }
    else
    {
        success = false;
        value = TEXT("");
    }
}

void UMasterHttpRequestBPLibrary::DecodeNestedJson(FString jsonString, FString key, bool& success, TArray<FNestedJson>& keyValuePairArray)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(jsonString);
    success = FJsonSerializer::Deserialize(Reader, JsonObject);

    if (success && JsonObject.IsValid())
    {
        TArray<FString> keys;
        key.ParseIntoArray(keys, TEXT("."), true);

        for (auto k : keys)
        {
            if (JsonObject->HasField(k))
            {
                if (JsonObject->GetObjectField(k).IsValid())
                {
                    JsonObject = JsonObject->GetObjectField(k);
                }
                else
                {
                    success = false;
                    keyValuePairArray.Empty();
                    return;
                }
            }
            else
            {
                success = false;
                keyValuePairArray.Empty();
                return;
            }
        }

        for (const auto& pair : JsonObject->Values)
        {
            FNestedJson kvPair;
            kvPair.KeyValuePair.Key = pair.Key;
            kvPair.bIsNested = pair.Value->Type == EJson::Object || pair.Value->Type == EJson::Array;

            if (kvPair.bIsNested)
            {
                FString OutNestedJsonString;
                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutNestedJsonString);

                if (pair.Value->Type == EJson::Object)
                {
                    FJsonSerializer::Serialize(pair.Value->AsObject().ToSharedRef(), Writer);
                }
                else if (pair.Value->Type == EJson::Array)
                {
                    Writer->WriteArrayStart();
                    for (const auto& ArrayVal : pair.Value->AsArray())
                    {
                        if (ArrayVal->Type == EJson::Object)
                        {
                            FJsonSerializer::Serialize(ArrayVal->AsObject().ToSharedRef(), Writer);
                        }
                        else
                        {
                            UMasterHttpRequestBPLibrary::WriteJsonValue(Writer, ArrayVal);
                        }
                    }
                    Writer->WriteArrayEnd();
                }

                kvPair.KeyValuePair.Value = OutNestedJsonString;
                kvPair.NestedKey = FString::Printf(TEXT("%s.%s"), *key, *pair.Key);
            }
            else
            {
                kvPair.KeyValuePair.Value = pair.Value->AsString();
                kvPair.NestedKey = TEXT("");
            }

            keyValuePairArray.Add(kvPair);
        }

        success = true;
    }
    else
    {
        success = false;
        keyValuePairArray.Empty();
    }
}

void UMasterHttpRequestBPLibrary::WriteJsonValue(TSharedRef<TJsonWriter<>> Writer, TSharedPtr<FJsonValue> JsonVal)
{
    switch (JsonVal->Type)
    {
        case EJson::String:
            Writer->WriteValue(JsonVal->AsString());
            break;

        case EJson::Number:
            Writer->WriteValue(JsonVal->AsNumber());
            break;

        case EJson::Boolean:
            Writer->WriteValue(JsonVal->AsBool());
            break;
        case EJson::Object:
            FJsonSerializer::Serialize(JsonVal->AsObject().ToSharedRef(), Writer);
            break;
        case EJson::Array:
            Writer->WriteArrayStart();
            for (const auto& ArrayVal : JsonVal->AsArray())
            {
                UMasterHttpRequestBPLibrary::WriteJsonValue(Writer, ArrayVal);
            }
            Writer->WriteArrayEnd();
            break;
        default:
            break;
    }
}
