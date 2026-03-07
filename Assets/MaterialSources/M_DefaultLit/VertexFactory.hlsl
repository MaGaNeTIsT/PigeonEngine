Output.Pos     = TransformObjectToClip(ScalePosition(Input.Position0.xyz, POSITION_SCALE));
Output.Normal  = TransformObjectToWorldNormal(Input.Normal0.xyz);
Output.Tangent = TransformObjectToWorldNormal(Input.Tangent0.xyz);
Output.UV      = Input.Texcoord0.xy;
