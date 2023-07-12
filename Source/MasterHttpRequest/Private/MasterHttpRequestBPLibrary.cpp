#include "MasterHttpRequestBPLibrary.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Async/Async.h"

void UMasterHttpRequestBPLibrary::MasterRequest(FString url, E_RequestType_CPP httpMethod, FRequestReturn callback)
{
    // Call the overloaded version with empty arrays for bodyPayload and headers
    MasterRequestWithPayloadAndHeaders(url, httpMethod, TArray<FKeyValuePair>(), TArray<FKeyValuePair>(), callback);
}

void UMasterHttpRequestBPLibrary::MasterRequestWithPayloadAndHeaders(FString url, E_RequestType_CPP httpMethod, TArray<FKeyValuePair> bodyPayload, TArray<FKeyValuePair> headers, FRequestReturn callback)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();

    HttpRequest->SetURL(url);
    HttpRequest->SetVerb(StaticEnum<E_RequestType_CPP>()->GetDisplayNameTextByValue(static_cast<int64>(httpMethod)).ToString());

    // Set the headers on the HttpRequest
    for (auto& KeyValuePair : headers)
    {
        HttpRequest->SetHeader(KeyValuePair.Key, KeyValuePair.Value);
    }

    // Create a JSON object and populate it with the key-value pairs from bodyPayload
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    for (auto& KeyValuePair : bodyPayload)
    {
        JsonObject->SetStringField(KeyValuePair.Key, KeyValuePair.Value);
    }

    // Serialize the JSON object into a string
    FString BodyData;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyData);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    // Set the body content and the Content-Type header of the HTTP request
    HttpRequest->SetContentAsString(BodyData);
    if (HttpRequest->GetHeader(TEXT("Content-Type")).IsEmpty()) // Only set the Content-Type header if it hasn't been set already
    {
        HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    }

    HttpRequest->OnProcessRequestComplete().BindLambda([callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
    {
        FResponseData ReturnData;
        ReturnData.bSuccess = bWasSuccessful;
        if (Response.IsValid()) // Ensure that the response is valid
        {
            ReturnData.Data = Response->GetContentAsString();
            ReturnData.StatusCode = Response->GetResponseCode();
            ReturnData.ErrorMessage = Response->GetContentAsString(); // You may want to handle error messages differently
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

void UMasterHttpRequestBPLibrary::MasterRequestAsync(FString url, E_RequestType_CPP httpMethod, TArray<FKeyValuePair> bodyPayload, TArray<FKeyValuePair> headers, FRequestReturn callback)
{
    Async(EAsyncExecution::TaskGraph, [url, httpMethod, bodyPayload, headers, callback]() {
        MasterRequestWithPayloadAndHeaders(url, httpMethod, bodyPayload, headers, callback);
        //now handle the callback on the game thread
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
                        // Here assuming array elements are objects, need more checks if there can be other types
                        FJsonSerializer::Serialize(ArrayVal->AsObject().ToSharedRef(), Writer);
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
