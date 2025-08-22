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

FHttpHeaderEnumValue UMasterHttpRequestBPLibrary::MakeEnumHeader(EHttpHeaderKey Key, const FString& Value, const FString& CustomKey)
{
    FHttpHeaderEnumValue Header;
    Header.Key = Key;
    Header.Value = Value;
    Header.CustomKey = CustomKey;
    return Header;
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
    TArray<FHttpHeaderEnumValue> DefaultHeaders,
    TArray<FHttpKeyValue> CustomHeaders,
    TArray<FHttpKeyValue> QueryParams,
    TArray<FHttpKeyValue> Body,
    FHttpResponseDelegate Callback,
    FHttpOptions Options)
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

        // Merge default headers (enum-based), custom headers, and JSON defaults
        TMap<FString, FString> FinalHeaders;
        for (const auto& H : GetDefaultJsonHeaders())
        {
            FinalHeaders.Add(H.Key, H.Value);
        }
        for (const auto& H : DefaultHeaders)
        {
            FString KeyStr;
            switch (H.Key)
            {
                case EHttpHeaderKey::Authorization: KeyStr = TEXT("Authorization"); break;
                case EHttpHeaderKey::UserAgent: KeyStr = TEXT("User-Agent"); break;
                case EHttpHeaderKey::AcceptLanguage: KeyStr = TEXT("Accept-Language"); break;
                case EHttpHeaderKey::CacheControl: KeyStr = TEXT("Cache-Control"); break;
                case EHttpHeaderKey::Cookie: KeyStr = TEXT("Cookie"); break;
                case EHttpHeaderKey::Referer: KeyStr = TEXT("Referer"); break;
                case EHttpHeaderKey::Custom: KeyStr = H.CustomKey; break;
                default: continue;
            }
            if (!KeyStr.IsEmpty())
            {
                FinalHeaders.Add(KeyStr, H.Value);
            }
        }
        for (const auto& H : CustomHeaders)
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
                LogDebugInfo(FinalURL, Method, QueryParams, CustomHeaders, Body, RespData, Options);
            }
            Callback.ExecuteIfBound(RespData);
        });

        HttpRequest->ProcessRequest();
    };

    Async(EAsyncExecution::TaskGraph, RequestLambda);
}

void UMasterHttpRequestBPLibrary::DecodeJson(const FString& JsonString, const FString& KeyPath, EJsonDecodeResult& Result, FString& Value, TArray<FHttpKeyValue>& ObjectFields, TArray<FString>& ArrayValues)
{
    Result = EJsonDecodeResult::Failed;
    Value = TEXT("");
    ObjectFields.Empty();
    ArrayValues.Empty();

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
        return;

    if (KeyPath.IsEmpty())
    {
        // If KeyPath is empty, decode the root object and fill ObjectFields
        if (JsonObject->Values.Num() > 0)
        {
            for (const auto& Pair : JsonObject->Values)
            {
                FString FieldValue;
                switch (Pair.Value->Type)
                {
                    case EJson::String:
                        FieldValue = Pair.Value->AsString();
                        break;
                    case EJson::Number:
                        FieldValue = FString::SanitizeFloat(Pair.Value->AsNumber());
                        break;
                    case EJson::Boolean:
                        FieldValue = Pair.Value->AsBool() ? TEXT("true") : TEXT("false");
                        break;
                    case EJson::Object:
                    {
                        FString ObjectJson;
                        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ObjectJson);
                        FJsonSerializer::Serialize(Pair.Value->AsObject().ToSharedRef(), Writer);
                        FieldValue = ObjectJson;
                        break;
                    }
                    case EJson::Array:
                    {
                        FString ArrayJson;
                        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ArrayJson);
                        FJsonSerializer::Serialize(Pair.Value->AsArray(), Writer);
                        FieldValue = ArrayJson;
                        break;
                    }
                    default:
                        FieldValue = TEXT("");
                        break;
                }
                ObjectFields.Add(MakeKeyValue(Pair.Key, FieldValue));
            }
            Result = EJsonDecodeResult::ObjectFields;
        }
        return;
    }

    TArray<FString> Keys;
    KeyPath.ParseIntoArray(Keys, TEXT("."), true);
    TSharedPtr<FJsonObject> CurrentObj = JsonObject;
    TSharedPtr<FJsonValue> CurrentVal;
    for (int32 i = 0; i < Keys.Num(); ++i)
    {
        CurrentVal = CurrentObj->TryGetField(Keys[i]);
        if (!CurrentVal.IsValid())
            return;
        if (i == Keys.Num() - 1)
        {
            switch (CurrentVal->Type)
            {
                case EJson::String:
                    Value = CurrentVal->AsString();
                    Result = EJsonDecodeResult::Value;
                    break;
                case EJson::Number:
                    Value = FString::SanitizeFloat(CurrentVal->AsNumber());
                    Result = EJsonDecodeResult::Value;
                    break;
                case EJson::Boolean:
                    Value = CurrentVal->AsBool() ? TEXT("true") : TEXT("false");
                    Result = EJsonDecodeResult::Value;
                    break;
                case EJson::Object:
                {
                    TSharedPtr<FJsonObject> Obj = CurrentVal->AsObject();
                    for (const auto& Pair : Obj->Values)
                    {
                        FString FieldValue;
                        if (Pair.Value->Type == EJson::String)
                            FieldValue = Pair.Value->AsString();
                        else if (Pair.Value->Type == EJson::Number)
                            FieldValue = FString::SanitizeFloat(Pair.Value->AsNumber());
                        else if (Pair.Value->Type == EJson::Boolean)
                            FieldValue = Pair.Value->AsBool() ? TEXT("true") : TEXT("false");
                        else
                        {
                            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&FieldValue);
                            if (Pair.Value->Type == EJson::Object)
                                FJsonSerializer::Serialize(Pair.Value->AsObject().ToSharedRef(), Writer);
                            else if (Pair.Value->Type == EJson::Array)
                                FJsonSerializer::Serialize(Pair.Value->AsArray(), Writer);
                        }
                        ObjectFields.Add(MakeKeyValue(Pair.Key, FieldValue));
                    }
                    Result = EJsonDecodeResult::ObjectFields;
                    break;
                }
                case EJson::Array:
                {
                    const TArray<TSharedPtr<FJsonValue>>& Arr = CurrentVal->AsArray();
                    for (const auto& Elem : Arr)
                    {
                        FString ObjectJson;
                        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ObjectJson);
                        FJsonSerializer::Serialize(Elem->AsObject().ToSharedRef(), Writer);
                        ArrayValues.Add(ObjectJson);
                    }
                    Result = EJsonDecodeResult::ArrayValues;
                    break;
                }
                default:
                    Result = EJsonDecodeResult::Failed;
                    break;
            }
        }
        else
        {
            if (CurrentVal->Type == EJson::Object)
                CurrentObj = CurrentVal->AsObject();
            else
                return;
        }
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

