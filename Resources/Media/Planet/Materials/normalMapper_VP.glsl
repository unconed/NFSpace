void main()	{
    gl_Position = ftransform();																
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_TexCoord[1] = gl_MultiTexCoord1;
}
