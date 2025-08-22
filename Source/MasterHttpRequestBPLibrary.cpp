void UMasterHttpRequestBPLibrary::DecodeJson(const FString& JsonString, const FString& KeyPath, bool& bSuccess, FString& Value, TArray<FString>& ArrayValues)
{
    bSuccess = false;
    Value = TEXT("");
    ArrayValues.Empty();

    // Defensive: ensure input is valid JSON object, not just a fragment
    FString ValidJsonString = JsonString;
    if (!ValidJsonString.TrimStart().StartsWith("{"))
    {
        ValidJsonString = "{" + ValidJsonString + "}";
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ValidJsonString);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
        return;

    if (KeyPath.IsEmpty())
    {
        // If KeyPath is empty, just encode the root object as a JSON string and add to ArrayValues
        FString RootJson;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RootJson);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
        ArrayValues.Add(RootJson);
        bSuccess = true;
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
                    bSuccess = true;
                    break;
                case EJson::Number:
                    Value = FString::SanitizeFloat(CurrentVal->AsNumber());
                    bSuccess = true;
                    break;
                case EJson::Boolean:
                    Value = CurrentVal->AsBool() ? TEXT("true") : TEXT("false");
                    bSuccess = true;
                    break;
                case EJson::Array:
                {
                    const TArray<TSharedPtr<FJsonValue>>& Arr = CurrentVal->AsArray();
                    for (const auto& Elem : Arr)
                    {
                        FString ElemJson;
                        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ElemJson);
                        if (Elem->Type == EJson::Object)
                            FJsonSerializer::Serialize(Elem->AsObject().ToSharedRef(), Writer);
                        else if (Elem->Type == EJson::Array)
                            FJsonSerializer::Serialize(Elem->AsArray(), Writer);
                        else if (Elem->Type == EJson::String)
                            ElemJson = Elem->AsString();
                        else if (Elem->Type == EJson::Number)
                            ElemJson = FString::SanitizeFloat(Elem->AsNumber());
                        else if (Elem->Type == EJson::Boolean)
                            ElemJson = Elem->AsBool() ? TEXT("true") : TEXT("false");
                        ArrayValues.Add(ElemJson);
                    }
                    bSuccess = true;
                    break;
                }
                default:
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
