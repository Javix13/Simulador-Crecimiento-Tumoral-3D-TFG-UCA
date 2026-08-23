Shader "Custom/VoxelIndirect"
{
    Properties
    {
        _BaseColor ("Base Color", Color) = (1,1,1,1)
    }

    SubShader
    {
        Tags { "RenderType"="Opaque" "Queue"="Geometry" }

        Pass
        {
            Cull Back
            ZWrite On

            HLSLPROGRAM
            #pragma target 4.5
            #pragma vertex vert
            #pragma fragment frag

            #include "UnityCG.cginc"

            struct VoxelRenderData
            {
                float4 posScale;
                float4 color;
            };

            StructuredBuffer<VoxelRenderData> _VoxelBuffer;
            float4 _BaseColor;

            struct appdata
            {
                float3 vertex : POSITION;
                uint instanceID : SV_InstanceID;
            };

            struct v2f
            {
                float4 pos : SV_POSITION;
                float4 color : COLOR0;
            };

            v2f vert(appdata v)
            {
                v2f o;

                VoxelRenderData data = _VoxelBuffer[v.instanceID];
                float3 worldPos = data.posScale.xyz + v.vertex * data.posScale.w;

                o.pos = UnityWorldToClipPos(float4(worldPos, 1.0));
                o.color = data.color * _BaseColor;

                return o;
            }

            fixed4 frag(v2f i) : SV_Target
            {
                return i.color;
            }
            ENDHLSL
        }
    }
}