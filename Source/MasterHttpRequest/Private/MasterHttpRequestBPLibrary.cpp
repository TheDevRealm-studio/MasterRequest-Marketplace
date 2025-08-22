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
#include "HAL/PlatformFilemanager.h"

FHttpKeyValue UMasterHttpRequestBPLibrary::MakeKeyValue(const FString& Key, const FString& Value)
{
    FHttpKeyValue Pair;
    Pair.Key = Key;
    Pair.Value = Value;
    return Pair;
}

TArray<FHttpKeyValue> UMasterHttpRequestBPLibrary::GetDefaultJsonHeaders()
{
    TArray<FHttpKeyValue> Headers;
    Headers.Add(MakeKeyValue(TEXT("Content-Type"), TEXT("application/json")));
    Headers.Add(MakeKeyValue(TEXT("Accept"), TEXT("application/json")));
    return Headers;
}

void UMasterHttpRequestBPLibrary::SendHttpRequest(
    const FString& URL,
    EHttpMethod Method,
    const TArray<FHttpKeyValue>& QueryParams,
    const TArray<FHttpKeyValue>& Headers,
    const TArray<FHttpKeyValue>& Body,
    FHttpResponseDelegate Callback,
    const FHttpOptions& Options)
{
    auto RequestLambda = [=]() {
        FString FinalURL = URL;
        if (QueryParams.Num() > 0)
        {
            TArray<FString> QueryArray;
            for (const auto& Param : QueryParams)
            {
                QueryArray.Add(FGenericPlatformHttp::UrlEncode(Param.Key) + TEXT("=") + FGenericPlatformHttp::UrlEncode(Param.Value));
            }
            FinalURL += (URL.Contains(TEXT("?")) ? TEXT("&") : TEXT("?")) + FString::Join(QueryArray, TEXT("&"));
        }

        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
        HttpRequest->SetURL(FinalURL);
        FString Verb;
        switch (Method)
        {
            case EHttpMethod::GET: Verb = TEXT("GET"); break;
            case EHttpMethod::POST: Verb = TEXT("POST"); break;
            case EHttpMethod::PUT: Verb = TEXT("PUT"); break;
            case EHttpMethod::DELETE: Verb = TEXT("DELETE"); break;
            case EHttpMethod::PATCH: Verb = TEXT("PATCH"); break;
            default: Verb = TEXT("GET"); break;
        }
        HttpRequest->SetVerb(Verb);

        // Merge default headers with user headers
        TMap<FString, FString> FinalHeaders;
        for (const auto& H : GetDefaultJsonHeaders())
        {
            FinalHeaders.Add(H.Key, H.Value);
        }
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
                JsonObject->SetStringField(KV.Key, KV.Value);
            }
            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
            FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
            HttpRequest->SetContentAsString(BodyString);
        }

        HttpRequest->SetTimeout(Options.TimeoutSeconds);
        // SSL options: Unreal does not expose direct self-signed handling, but can be extended here if needed

        HttpRequest->OnProcessRequestComplete().BindLambda([=](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {
            FHttpResponseSimple RespData;
            RespData.bSuccess = bWasSuccessful && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode());
            RespData.Data = Response.IsValid() ? Response->GetContentAsString() : TEXT("");
            RespData.StatusCode = Response.IsValid() ? Response->GetResponseCode() : -1;
            RespData.ErrorMessage = bWasSuccessful ? TEXT("") : (Response.IsValid() ? Response->GetContentAsString() : TEXT("No response"));
            if (Response.IsValid())
            {
                for (const auto& Header : Response->GetAllHeaders())
                {
                    FString Key, Value;
                    if (Header.Split(TEXT(": "), &Key, &Value))
                    {
                        RespData.Headers.Add(MakeKeyValue(Key, Value));
                    }
                }
            }
            if (Options.bDebug)
            {
                LogDebugInfo(FinalURL, Method, QueryParams, Headers, Body, RespData, Options);
            }
            Callback.ExecuteIfBound(RespData);
        });

        HttpRequest->ProcessRequest();
    };

    Async(EAsyncExecution::TaskGraph, RequestLambda);
}

void UMasterHttpRequestBPLibrary::DecodeJson(const FString& JsonString, const FString& KeyPath, bool& bSuccess, FString& Value)
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

void UMasterHttpRequestBPLibrary::LogDebugInfo(const FString& URL, EHttpMethod Method, const TArray<FHttpKeyValue>& QueryParams, const TArray<FHttpKeyValue>& Headers, const TArray<FHttpKeyValue>& Body, const FHttpResponseSimple& Response, const FHttpOptions& Options)
{
    FString DebugInfo;
    DebugInfo += TEXT("URL: ") + URL + TEXT("\n");
    DebugInfo += TEXT("Method: ") + StaticEnum<EHttpMethod>()->GetDisplayNameTextByValue(static_cast<int64>(Method)).ToString() + TEXT("\n");
    DebugInfo += TEXT("Query Params:\n");
    for (const auto& Q : QueryParams)
    {
        DebugInfo += Q.Key + TEXT("=") + Q.Value + TEXT("\n");
    }
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
    DebugInfo += TEXT("\nResponse:\n");
    DebugInfo += TEXT("Status Code: ") + FString::FromInt(Response.StatusCode) + TEXT("\n");
    DebugInfo += TEXT("Success: ") + (Response.bSuccess ? FString(TEXT("true")) : FString(TEXT("false"))) + TEXT("\n");
    DebugInfo += TEXT("Error Message: ") + Response.ErrorMessage + TEXT("\n");
    DebugInfo += TEXT("Data: ") + Response.Data + TEXT("\n");
    DebugInfo += TEXT("Response Headers:\n");
    for (const auto& H : Response.Headers)
    {
        DebugInfo += H.Key + TEXT(": ") + H.Value + TEXT("\n");
    }

    FString Timestamp = FDateTime::Now().ToString(TEXT("%Y-%m-%d_%H-%M-%S"));
    FString SlugifiedURL = URL;
    SlugifiedURL.ReplaceInline(TEXT("//"), TEXT("_"));
    SlugifiedURL.ReplaceInline(TEXT("/"), TEXT("_"));
    SlugifiedURL.ReplaceInline(TEXT(":"), TEXT("_"));
    FString FileName = SlugifiedURL + TEXT("-") + StaticEnum<EHttpMethod>()->GetDisplayNameTextByValue(static_cast<int64>(Method)).ToString() + TEXT("-") + Timestamp + TEXT(".txt");
    FString FileDirectory = FPaths::ProjectSavedDir() + TEXT("/DebugHTTP");
    FString FullPath = FileDirectory + TEXT("/") + FileName;

    if (!FPaths::DirectoryExists(FileDirectory))
    {
        FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*FileDirectory);
    }

    FFileHelper::SaveStringToFile(DebugInfo, *FullPath);
}

