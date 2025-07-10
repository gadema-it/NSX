#version 330

//in int gl_PrimitiveID ;

uniform vec3 color;

out vec4 fColor;


void main()
{
   // int vertexColor = gl_PrimitiveID;
   // float r = (vertexColor & 0x000000FF) >> 0;
   // float g =  (vertexColor & 0x0000FF00) >> 8;
   // float b =  (vertexColor & 0x00FF0000) >> 16;
    //fColor = vec4(r, g, b, 1);

    //fColor = vec4(vertexColor/255, vertexColor/255, vertexColor/255, 1);

    //fColor = vec4((vertexColor & 0x000000FF) >> 0, (vertexColor & 0x0000FF00) >> 8, (vertexColor & 0x00FF0000) >> 16, (vertexColor & 0xFF000000)>> 24);


//    if(gl_PrimitiveID == 0)
//       fColor = vec4(1, 0, 0, 1);
//    else if (gl_PrimitiveID == 1)
//        fColor = vec4(0, 1, 0, 1);
//    else
//        fColor = vec4(gl_PrimitiveID/255.0f, 0, 0, 1);

   fColor = vec4(color, 1);
}
