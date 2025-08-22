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
#include "HAL/PlatformFilemanager.h"

FKeyValuePair UMasterHttpRequestBPLibrary::MakeHeader(const FString& Key, const FString& Value)
{
    FKeyValuePair Pair;
    Pair.Key = Key;
    Pair.Value = Value;
    return Pair;
}

void UMasterHttpRequestBPLibrary::SendHttpRequest(const FString& URL, E_RequestType_CPP Method, const TArray<FKeyValuePair>& Headers, const TArray<FKeyValuePair>& Body, FRequestReturn Callback, bool bDebug, const FHttpRequestOptions& Options)
{
    auto RequestLambda = [=]() {
        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
        HttpRequest->SetURL(URL);
        FString Verb = StaticEnum<E_RequestType_CPP>()->GetDisplayNameTextByValue(static_cast<int64>(Method)).ToString();
        HttpRequest->SetVerb(Verb);

        // Default headers for Laravel
        TMap<FString, FString> FinalHeaders;
        FinalHeaders.Add(TEXT("Content-Type"), TEXT("application/json"));
        FinalHeaders.Add(TEXT("Accept"), TEXT("application/json"));
        for (const auto& H : Headers)
        {
            FinalHeaders.Add(H.Key, H.Value);
        }
        for (const auto& Pair : FinalHeaders)
        {
            HttpRequest->SetHeader(Pair.Key, Pair.Value);
        }

        FString BodyString;
        if (Verb == TEXT("POST") || Verb == TEXT("PUT") || Verb == TEXT("PATCH"))
        {
            TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
            for (const auto& KV : Body)
            {
                JsonObject->SetStringField(KV.Key, Options.bEncodePayload ? FGenericPlatformHttp::UrlEncode(KV.Value) : KV.Value);
            }
            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
            FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
            HttpRequest->SetContentAsString(BodyString);
        }

        HttpRequest->SetTimeout(Options.TimeoutSeconds);

        HttpRequest->OnProcessRequestComplete().BindLambda([=](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {
            FResponseData RespData;
            RespData.bSuccess = bWasSuccessful && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode());
            RespData.Data = Response.IsValid() ? Response->GetContentAsString() : TEXT("");
            RespData.StatusCode = Response.IsValid() ? Response->GetResponseCode() : -1;
            RespData.ErrorMessage = bWasSuccessful ? TEXT("") : (Response.IsValid() ? Response->GetContentAsString() : TEXT("No response"));

            if (bDebug)
            {
                SaveDebugToFile(URL, Method, Headers, Body, Response, RespData);
            }
            Callback.ExecuteIfBound(RespData);
        });

        HttpRequest->ProcessRequest();
    };

    if (Options.bAsync)
    {
        Async(EAsyncExecution::TaskGraph, RequestLambda);
    }
    else
    {
        RequestLambda();
    }
}

void UMasterHttpRequestBPLibrary::DecodeJsonNested(const FString& JsonString, const FString& KeyPath, bool& bSuccess, FString& Value)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    bSuccess = FJsonSerializer::Deserialize(Reader, JsonObject);
    Value = TEXT("");
    if (bSuccess && JsonObject.IsValid())
    {
        TArray<FString> Keys;
        KeyPath.ParseIntoArray(Keys, TEXT("."), true);
        TSharedPtr<FJsonObject> CurrentObj = JsonObject;
        for (int32 i = 0; i < Keys.Num(); ++i)
        {
            if (i == Keys.Num() - 1)
            {
                if (!CurrentObj->TryGetStringField(Keys[i], Value))
                {
                    bSuccess = false;
                }
            }
            else
            {
                TSharedPtr<FJsonObject> NextObj;
                TSharedPtr<FJsonValue> Val = CurrentObj->TryGetField(Keys[i]);
                if (Val.IsValid() && Val->Type == EJson::Object)
                {
                    NextObj = Val->AsObject();
                    CurrentObj = NextObj;
                }
                else
                {
                    bSuccess = false;
                    break;
                }
            }
        }
    }
    else
    {
        bSuccess = false;
    }
}

void UMasterHttpRequestBPLibrary::SaveDebugToFile(const FString& URL, E_RequestType_CPP Method, const TArray<FKeyValuePair>& Headers, const TArray<FKeyValuePair>& Body, TSharedPtr<IHttpResponse> Response, const FResponseData& ReturnData)
{
    FString DebugInfo;
    DebugInfo += TEXT("URL: ") + URL + TEXT("\n");
    DebugInfo += TEXT("Method: ") + StaticEnum<E_RequestType_CPP>()->GetDisplayNameTextByValue(static_cast<int64>(Method)).ToString() + TEXT("\n");
    DebugInfo += TEXT("Headers:\n");
    for (const auto& H : Headers)
    {
        DebugInfo += H.Key + TEXT(": ") + H.Value + TEXT("\n");
    }
    DebugInfo += TEXT("Body:\n");
    for (const auto& B : Body)
    {
        DebugInfo += B.Key + TEXT(": ") + B.Value + TEXT("\n");
    }
    if (Response.IsValid())
    {
        DebugInfo += TEXT("\nServer Headers:\n");
        for (const auto& Header : Response->GetAllHeaders())
        {
            DebugInfo += Header + TEXT("\n");
        }
        DebugInfo += TEXT("\nServer Payload:\n") + Response->GetContentAsString() + TEXT("\n");
        DebugInfo += TEXT("Status Code: ") + FString::FromInt(Response->GetResponseCode()) + TEXT("\n");
    }
    DebugInfo += TEXT("\nResponse Data:\n") + ReturnData.Data + TEXT("\n");
    DebugInfo += FString::Printf(TEXT("Success: %s\n"), ReturnData.bSuccess ? TEXT("true") : TEXT("false"));
    DebugInfo += TEXT("Error Message: ") + ReturnData.ErrorMessage + TEXT("\n");

    FString Timestamp = FDateTime::Now().ToString(TEXT("%Y-%m-%d_%H-%M-%S"));
    FString SlugifiedURL = URL;
    SlugifiedURL.ReplaceInline(TEXT("//"), TEXT("_"));
    SlugifiedURL.ReplaceInline(TEXT("/"), TEXT("_"));
    SlugifiedURL.ReplaceInline(TEXT(":"), TEXT("_"));
    FString FileName = SlugifiedURL + TEXT("-") + StaticEnum<E_RequestType_CPP>()->GetDisplayNameTextByValue(static_cast<int64>(Method)).ToString() + TEXT("-") + Timestamp + TEXT(".txt");
    FString FileDirectory = FPaths::ProjectSavedDir() + TEXT("/DebugHTTP");
    FString FullPath = FileDirectory + TEXT("/") + FileName;

    if (!FPaths::DirectoryExists(FileDirectory))
    {
        FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*FileDirectory);
    }

    if (FFileHelper::SaveStringToFile(DebugInfo, *FullPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Saved HTTP debug info to %s"), *FullPath);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save HTTP debug info to %s"), *FullPath);
    }
}

