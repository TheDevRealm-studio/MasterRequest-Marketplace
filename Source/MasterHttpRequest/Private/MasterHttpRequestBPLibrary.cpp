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

void UMasterHttpRequestBPLibrary::MasterRequest(FString url, E_RequestType_CPP httpMethod, FRequestReturn callback)
{
    // Call the overloaded version with empty arrays for bodyPayload and headers
     MasterRequestWithPayloadAndHeaders(url, httpMethod, TArray<FKeyValuePair>(), TArray<FKeyValuePair>(), callback, false);
}

void UMasterHttpRequestBPLibrary::MasterRequestWithPayloadAndHeaders(FString url, E_RequestType_CPP httpMethod, TArray<FKeyValuePair> bodyPayload, TArray<FKeyValuePair> headers, FRequestReturn callback, bool debugResponse)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();

    HttpRequest->SetURL(url);
    HttpRequest->SetVerb(StaticEnum<E_RequestType_CPP>()->GetDisplayNameTextByValue(static_cast<int64>(httpMethod)).ToString());

    // Set the headers on the HttpRequest
    for (auto& KeyValuePair : headers)
    {
        FString EncodedValue = FGenericPlatformHttp::UrlEncode(KeyValuePair.Value);
        HttpRequest->SetHeader(KeyValuePair.Key, EncodedValue);
    }

    // Create a JSON object and populate it with the key-value pairs from bodyPayload
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    for (auto& KeyValuePair : bodyPayload)
    {
        if (!KeyValuePair.Key.IsEmpty() && !KeyValuePair.Value.IsEmpty())
        {
            FString EncodedValue = FGenericPlatformHttp::UrlEncode(KeyValuePair.Value);
            JsonObject->SetStringField(KeyValuePair.Key, EncodedValue);
        }
    }

    // Serialize the JSON object into a string
    FString BodyData;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyData);

    if (JsonObject.IsValid() && JsonObject->Values.Num() > 0) // Only attempt to serialize if the JSON object is valid and has values
    {
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Error: JSON object is invalid or empty."));
    }

    // Set the body content and the Content-Type header of the HTTP request
    HttpRequest->SetContentAsString(BodyData);
    if (HttpRequest->GetHeader(TEXT("Content-Type")).IsEmpty()) // Only set the Content-Type header if it hasn't been set already
    {
        HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    }

    HttpRequest->OnProcessRequestComplete().BindLambda([callback, debugResponse, url, httpMethod, JsonObject](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
    {
        FResponseData ReturnData;
        ReturnData.bSuccess = bWasSuccessful;
        if (Response.IsValid()) // Ensure that the response is valid
        {
            ReturnData.Data = Response->GetContentAsString();
            ReturnData.StatusCode = Response->GetResponseCode();
            ReturnData.ErrorMessage = Response->GetContentAsString(); // You may want to handle error messages differently

            if (debugResponse) {
                 // Debugging: print the response information to a text file
                FString ResponseInfo;
                ResponseInfo += "\n\nServer Header:\n";
                for (auto& Header : Response->GetAllHeaders()) {
                    ResponseInfo += Header + "\n";
                }

                // Get the payload
                ResponseInfo += "\n\nServer Payload:\n" + Response->GetContentAsString();
                ResponseInfo += "\n\nServer Response:\n" + ReturnData.Data;
                ResponseInfo += "\n\nStatus code:\n" + FString::FromInt(ReturnData.StatusCode);

                if (JsonObject.IsValid() && JsonObject->Values.Num() > 0) {
                    // Add separator
                    ResponseInfo += "\n\n--/--\n\n";

                    // Get the client payload and append it to the response information
                    ResponseInfo += "Client Payload:\n";
                    for (auto& Field : JsonObject->Values) {
                        ResponseInfo += Field.Key + ": " + Field.Value->AsString() + "\n";
                    }
                    ResponseInfo += "\n";
                }


                // Generate a timestamp for the filename dd-mm-yyyy:hh-mm-ss
                FString Timestamp = FDateTime::Now().ToString(TEXT("%d-%m-%Y_%H-%M-%S"));

                // Create a new variable to store the modified URL
                FString SlugifiedURL = url;
                // Make the URL a valid filename by replacing ":" with "_"
                SlugifiedURL.ReplaceInline(TEXT("//"), TEXT(" "));
                SlugifiedURL.ReplaceInline(TEXT("/"), TEXT(" "));
                SlugifiedURL.ReplaceInline(TEXT(":"), TEXT("_"));
                SlugifiedURL.ReplaceInline(TEXT("http"), TEXT("http_"));
                SlugifiedURL.ReplaceInline(TEXT("https"), TEXT("https_"));

                // Set the filename as URL-method-timestamp.txt
                FString FileName = SlugifiedURL + "-" + StaticEnum<E_RequestType_CPP>()->GetDisplayNameTextByValue(static_cast<int64>(httpMethod)).ToString() + "-" + Timestamp + ".txt";
                FString FileDirectory = FPaths::ProjectSavedDir() + "/DebugHTTP";
                FString FullPath = FileDirectory + "/" + FileName;

                // Ensure that the directory exists
                if (!FPaths::DirectoryExists(FileDirectory)) {
                    FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*FileDirectory);
                }

                // Write the response information to the file
                if (FFileHelper::SaveStringToFile(ResponseInfo, *FullPath)) {
                    UE_LOG(LogTemp, Warning, TEXT("Saved response information to %s"), *FullPath);
                } else {
                    UE_LOG(LogTemp, Error, TEXT("Failed to save response information to %s"), *FullPath);
                }
            }
        }
        else
        {
            ReturnData.ErrorMessage = "No valid response.";
            ReturnData.StatusCode = 0;
        }

        callback.ExecuteIfBound(ReturnData);
    });

    HttpRequest->ProcessRequest(); // Don't forget to actually send the request
}

void UMasterHttpRequestBPLibrary::MasterRequestAsync(FString url, E_RequestType_CPP httpMethod, TArray<FKeyValuePair> bodyPayload, TArray<FKeyValuePair> headers, FRequestReturn callback, bool debugResponse)
{
    Async(EAsyncExecution::TaskGraph, [=]() { // Capture 'url' by value
        MasterRequestWithPayloadAndHeaders(url, httpMethod, bodyPayload, headers, callback, debugResponse);
    });
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

                // Handle object and array separately
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
                            WriteJsonValue(Writer, ArrayVal);
                        }
                    }
                    Writer->WriteArrayEnd();
                }

                kvPair.KeyValuePair.Value = OutNestedJsonString;
                kvPair.NestedKey = FString::Printf(TEXT("%s.%s"), *key, *pair.Key);
            }
            else
            {
                kvPair.KeyValuePair.Value = pair.Value->AsString(); // assuming non-nested values are strings
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

// Helper function to write different types of Json Values
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
                WriteJsonValue(Writer, ArrayVal);
            }
            Writer->WriteArrayEnd();
            break;
        // More cases can be added as per requirements
        default:
            break;
    }
}
