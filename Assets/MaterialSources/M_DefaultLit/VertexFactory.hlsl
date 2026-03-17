Output.Pos     = TransformObjectToClip(ScalePosition(Input.Position0.xyz, POSITION_SCALE));
Output.Normal  = float4(TransformObjectToWorldNormal(Input.Normal0.xyz), 1);
Output.Tangent = float4(TransformObjectToWorldNormal(Input.Tangent0.xyz), 1);
Output.UV      = Input.Texcoord0.xy;
