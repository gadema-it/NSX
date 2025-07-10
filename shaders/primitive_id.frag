#version 420



//in int gl_PrimitiveID ;

uniform vec3 color;
uniform bool use_color;
out vec4 fColor;


void main()
{
   // int vertexColor = gl_PrimitiveID;
   // float r = (gl_PrimitiveID & 0x000000FF) >> 0;
   // float g =  (gl_PrimitiveID & 0x0000FF00) >> 8;
   // float b =  (gl_PrimitiveID & 0x00FF0000) >> 16;
   // float a =  (gl_PrimitiveID & 0xFF000000) >> 32;
    //fColor = vec4(r, g, b, a);

   // fColor = vec4(vertexColor/255, vertexColor/255, vertexColor/255, 1);

    //fColor = vec4((vertexColor & 0x000000FF) >> 0, (vertexColor & 0x0000FF00) >> 8, (vertexColor & 0x00FF0000) >> 16, (vertexColor & 0xFF000000)>> 24);


//    if(gl_PrimitiveID == 0)
//       fColor = vec4(1, 0, 0, 1);
//    else if (gl_PrimitiveID == 1)
//        fColor = vec4(0, 1, 0, 1);
//    else

   if (use_color) {
       fColor = vec4(color, 1);
   } else {
       //fColor = vec4(gl_PrimitiveID/255.0f, 0, 0, 1);
       fColor = vec4(gl_PrimitiveID & 0x000000FF, 0, 0, 1);
   }

   //fColor = vec4(color, 1);
   // fColor = vec4(1,0,0,1);
}
