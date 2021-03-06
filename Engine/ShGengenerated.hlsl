#line 1 "C:\\Users\\Senpai\\source\\repos\\PCG_and_graphics_stale_memes\\Engine\\ShGen\\genVS.hlsl"
cbuffer PerCameraBuffer : register ( b10 ) 
{ 
    matrix projectionMatrix ; 
} ; 

cbuffer PerFrameBuffer : register ( b11 ) 
{ 
    matrix viewMatrix ; 
    float dTime ; 
    float eTime ; 
    float2 padding ; 
} ; 

cbuffer WMBuffer : register ( b0 ) 
{ 
    matrix worldMatrix ; 
} ; 

#line 20
struct VertexInputType 
{ 
    float4 position : POSITION ; 
    
    
    float2 tex : TEXCOORD0 ; 
    
    
    
    float3 normal : NORMAL ; 
    
    
#line 34
    
    
#line 38
    
} ; 

struct PixelInputType 
{ 
    float4 position : SV_POSITION ; 
    
    
    float2 tex : TEXCOORD0 ; 
    
    
    
    float3 normal : NORMAL ; 
    
    
    
    float3 worldPos : WPOS ; 
    
    
#line 59
    
} ; 

#line 63
PixelInputType main ( VertexInputType input ) 
{ 
    PixelInputType output ; 
    
    
    output . worldPos = mul ( input . position , worldMatrix ) ; 
    output . position = mul ( output . worldPos , viewMatrix ) ; 
    
#line 73
    
    
    output . position = mul ( output . position , projectionMatrix ) ; 
    
    
    output . tex = input . tex ; 
    
    
    
    output . normal = mul ( input . normal , ( float3x3 ) worldMatrix ) ; 
    output . normal = normalize ( output . normal ) ; 
    
    
#line 88
    
    
    return output ; 
}  