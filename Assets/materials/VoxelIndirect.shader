Shader "Custom/VoxelIndirect"
{
    Properties
    {
        _BaseColor ("Base Color", Color) = (1,1,1,1)
    }

    SubShader
    {
        Tags
        {
            "RenderType" = "Opaque"
            "Queue" = "Geometry"
        }

        Pass
        {
            Cull Back
            ZWrite On

            HLSLPROGRAM

            #pragma target 4.5
            #pragma vertex vert
            #pragma fragment frag

            #include "UnityCG.cginc"

            // Soporte para RenderMeshIndirect
            #define UNITY_INDIRECT_DRAW_ARGS IndirectDrawIndexedArgs
            #include "UnityIndirect.cginc"

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
                // Inicializa los argumentos del renderizado indirecto
                InitIndirectDrawArgs(0);

                v2f o;

                // Obtener el ID real de la instancia
                uint instanceID = GetIndirectInstanceID(v.instanceID);

                // Obtener los datos de esta célula
                VoxelRenderData data = _VoxelBuffer[instanceID];

                // Posición y escala
                float3 worldPos =
                    data.posScale.xyz +
                    v.vertex * data.posScale.w;

                // Transformación a espacio de pantalla
                o.pos = UnityWorldToClipPos(
                    float4(worldPos, 1.0)
                );

                // Color de la célula
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