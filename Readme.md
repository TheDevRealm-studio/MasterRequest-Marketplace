# 🌐 Master HTTP Request Plugin

A powerful, Laravel-inspired HTTP request plugin for Unreal Engine 5.6+ that simplifies HTTP communication with elegant Blueprint integration and advanced debugging capabilities.

## ✨ Features

- **🚀 Laravel-Inspired API**: Clean, intuitive interface for HTTP requests
- **📋 Multiple HTTP Methods**: GET, POST, PUT, DELETE, PATCH support
- **🔧 Flexible Headers**: Enum-based common headers + custom headers
- **📝 Content Type Support**: JSON, XML, Form-encoded, and more
- **🐛 Advanced Debugging**: 3-level debug system with file logging
- **⚡ Quick Methods**: Simplified functions for common operations
- **🔍 Smart JSON Decoding**: Nested JSON parsing with multiple output types
- **📊 Rich Response Data**: Status codes, timing, headers, and more

## 📦 Installation

### 1. Copy Plugin Files
Drag the plugin files to the `Plugins` folder in your main project directory.

### 2. Clean Binaries and Intermediate
Delete the `Binaries` and `Intermediate` folders from your project directory.

### 3. Open the Project
Open your Unreal Engine project. The plugin will automatically be compiled and integrated.

## 🚀 Quick Start

### Basic GET Request
```cpp
// In Blueprints: Use "Quick Get" node
QuickGet("https://api.example.com/users", ResponseCallback);
```

### POST Request with JSON
```cpp
// In Blueprints: Use "Quick Post" node
FString JsonBody = TEXT("{\"name\":\"John\",\"email\":\"john@example.com\"}");
QuickPost("https://api.example.com/users", JsonBody, ResponseCallback);
```

### Advanced Request with Headers
```cpp
// Create headers array
TArray<FHttpHeaderEnumValue> Headers;
Headers.Add(MakeBearerToken("your-auth-token"));
Headers.Add(MakeApiKey("your-api-key"));

// Create body data
TArray<FHttpKeyValue> Body;
Body.Add(MakeKeyValue("name", "John Doe"));
Body.Add(MakeKeyValue("email", "john@example.com"));

// Configure options
FHttpOptions Options;
Options.DebugLevel = EDebugLevel::Detailed;
Options.ContentType = EContentType::ApplicationJson;
Options.TimeoutSeconds = 30;

// Send request
SendHttpRequest(
    "https://api.example.com/users",
    EHttpMethod::POST,
    Headers,
    {}, // Custom headers
    {}, // Query params
    Body,
    ResponseCallback,
    Options
);
```

## 🔧 HTTP Methods

| Method | Blueprint Node | Description |
|--------|----------------|-------------|
| `GET` | Send Http Request | Retrieve data |
| `POST` | Send Http Request | Create new resource |
| `PUT` | Send Http Request | Update entire resource |
| `PATCH` | Send Http Request | Partial update |
| `DELETE` | Send Http Request | Delete resource |

## 📋 Headers Made Easy

### Common Headers (Enum-Based)
- `Authorization` - For Bearer tokens, API keys
- `Content-Type` - Automatic based on options
- `Accept` - What content types you accept
- `User-Agent` - Identify your application
- `X-API-Key` - API key authentication
- `X-Auth-Token` - Custom auth tokens
- And many more...

### Helper Functions
```cpp
// Bearer token
FHttpHeaderEnumValue authHeader = MakeBearerToken("your-token");

// API Key
FHttpHeaderEnumValue apiHeader = MakeApiKey("your-api-key");

// Content Type
FHttpHeaderEnumValue contentHeader = MakeContentTypeHeader(EContentType::ApplicationJson);
```

## 🔍 JSON Decoding

The `DecodeJson` function provides powerful JSON parsing with multiple output types:

### Basic Usage
```cpp
EJsonDecodeResult Result;
FString Value;
TArray<FHttpKeyValue> ObjectFields;
TArray<FString> ArrayValues;

DecodeJson(JsonString, "user.name", Result, Value, ObjectFields, ArrayValues);
```

### Result Types
- **`Failed`** - JSON parsing failed
- **`Value`** - Single value extracted (string, number, boolean)
- **`ObjectFields`** - Object decomposed into key-value pairs
- **`ArrayValues`** - Array elements as strings

### KeyPath Examples
```cpp
// Get root object fields
DecodeJson(JsonString, "", Result, Value, ObjectFields, ArrayValues);

// Get nested value
DecodeJson(JsonString, "user.profile.name", Result, Value, ObjectFields, ArrayValues);

// Get array
DecodeJson(JsonString, "users", Result, Value, ObjectFields, ArrayValues);
```

## 🐛 Debug System

### Debug Levels

#### 1. **None** - No debugging
```cpp
Options.DebugLevel = EDebugLevel::None;
```

#### 2. **Basic** - Console logging
```cpp
Options.DebugLevel = EDebugLevel::Basic;
// Output: 🌐 HTTP POST: https://api.example.com/users | Status: 200 OK | Duration: 0.45s
```

#### 3. **Detailed** - Console + File logging
```cpp
Options.DebugLevel = EDebugLevel::Detailed;
// Creates organized log files in ProjectSaved/MasterHttpDebug/
```

#### 4. **Verbose** - Everything including response data
```cpp
Options.DebugLevel = EDebugLevel::Verbose;
// Includes full response headers and body content
```

### Debug File Output
Files are saved to: `YourProject/Saved/MasterHttpDebug/`

Filename format: `STATUS_URL_METHOD_TIMESTAMP_STATUSCODE.txt`

Example: `SUCCESS_api_example_com_users_POST_2025-08-22_14-30-15_200.txt`

## 📊 Response Data

The response object contains rich information:

```cpp
struct FHttpResponseSimple
{
    bool bSuccess;                    // Request succeeded
    FString Data;                     // Response body
    int32 StatusCode;                 // HTTP status code (200, 404, etc.)
    FString StatusText;               // Human-readable status
    FString ErrorMessage;             // Error description if failed
    TArray<FHttpKeyValue> Headers;    // Response headers
    float RequestDurationSeconds;     // How long the request took
    int32 ContentLength;              // Response size in bytes
    FString ContentType;              // Response content type
    FString URL;                      // Final URL (after redirects)
};
```

## 🔧 Content Types

| Enum Value | Content-Type Header |
|------------|-------------------|
| `ApplicationJson` | application/json |
| `ApplicationXml` | application/xml |
| `ApplicationFormEncoded` | application/x-www-form-urlencoded |
| `MultipartFormData` | multipart/form-data |
| `TextPlain` | text/plain |
| `TextHtml` | text/html |
| `TextXml` | text/xml |
| `Custom` | Your custom type |

## 🎯 Blueprint Categories

Functions are organized in Blueprint categories for easy access:

- **HTTP Request** - Main SendHttpRequest function
- **HTTP Request | Quick Methods** - QuickGet, QuickPost
- **HTTP Request | Helpers** - Helper functions and utilities

## 💡 Best Practices

### 1. Use Appropriate Debug Levels
- **Development**: `Detailed` or `Verbose`
- **Production**: `None` or `Basic`

### 2. Handle Response Properly
```cpp
// Check if request succeeded
if (Response.bSuccess && IsSuccessStatusCode(Response.StatusCode))
{
    // Process successful response
    DecodeJson(Response.Data, "data", Result, Value, ObjectFields, ArrayValues);
}
else
{
    // Handle error
    UE_LOG(LogTemp, Error, TEXT("HTTP Error: %s"), *Response.ErrorMessage);
}
```

### 3. Use Helper Functions
```cpp
// Instead of manual header creation
FHttpHeaderEnumValue Header;
Header.Key = EHttpHeaderKey::Authorization;
Header.Value = "Bearer " + Token;

// Use helper
FHttpHeaderEnumValue Header = MakeBearerToken(Token);
```

### 4. Set Appropriate Timeouts
```cpp
FHttpOptions Options;
Options.TimeoutSeconds = 30; // Default
Options.TimeoutSeconds = 60; // For slow APIs
Options.TimeoutSeconds = 10; // For fast APIs
```

## 🔍 Common Status Codes

| Code | Meaning | Description |
|------|---------|-------------|
| 200 | OK | Request successful |
| 201 | Created | Resource created successfully |
| 400 | Bad Request | Invalid request format |
| 401 | Unauthorized | Authentication required |
| 403 | Forbidden | Access denied |
| 404 | Not Found | Resource doesn't exist |
| 429 | Too Many Requests | Rate limited |
| 500 | Internal Server Error | Server error |

Use `IsSuccessStatusCode(StatusCode)` to check if a status code indicates success (2xx range).

## 🛠️ Building the Plugin for Production

### From Project
1. Copy plugin files to your project's `Plugins` folder
2. Delete `Binaries` and `Intermediate` folders
3. Open your project - plugin will auto-compile

### From Command Line
Navigate to: `C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles`

```shell
.\RunUAT.bat BuildPlugin -plugin="Path\To\Your\MasterHttpRequest.uplugin" -package="Output\Path" -TargetPlatforms=Win64
```

Replace paths with your specific locations.

## 📝 Examples

### API Authentication Flow
```cpp
// Step 1: Login
TArray<FHttpKeyValue> LoginBody;
LoginBody.Add(MakeKeyValue("username", "user@example.com"));
LoginBody.Add(MakeKeyValue("password", "password123"));

QuickPost("https://api.example.com/auth/login", LoginBody, LoginCallback);

// Step 2: Use token in subsequent requests
FHttpHeaderEnumValue AuthHeader = MakeBearerToken(ReceivedToken);
TArray<FHttpHeaderEnumValue> Headers;
Headers.Add(AuthHeader);

SendHttpRequest("https://api.example.com/protected-endpoint", EHttpMethod::GET, Headers, {}, {}, {}, DataCallback, Options);
```

### File Upload Simulation
```cpp
FHttpOptions Options;
Options.ContentType = EContentType::MultipartFormData;

TArray<FHttpKeyValue> FormData;
FormData.Add(MakeKeyValue("file_name", "document.pdf"));
FormData.Add(MakeKeyValue("file_data", Base64EncodedData));

SendHttpRequest("https://api.example.com/upload", EHttpMethod::POST, {}, {}, {}, FormData, UploadCallback, Options);
```

## 🆘 Troubleshooting

### Common Issues

1. **Plugin not compiling**: Ensure you're using UE 5.6+
2. **Headers not working**: Check enum values match your API requirements
3. **JSON parsing fails**: Verify JSON format with verbose debugging
4. **Timeout errors**: Increase timeout in Options
5. **SSL errors**: Check `bVerifySSL` option

### Getting Help

For additional support:
- Check debug logs in `ProjectSaved/MasterHttpDebug/`
- Use `EDebugLevel::Verbose` for detailed information
- Refer to [Unreal Engine Forums](https://forums.unrealengine.com/)

---

**Made with ❤️ by MJGT Studio**

*Simplifying HTTP requests in Unreal Engine, one request at a time.*
