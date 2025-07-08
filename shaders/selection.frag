#version 420

layout (binding = 0, r8)  uniform image1D select_image;

//in int gl_PrimitiveID ;
flat in float vertex_id;
in int gl_PrimitiveID;

out vec4 fColor;


uniform vec3 color;
uniform int selection_mode;



void main()
{

   // int vertexColor = gl_PrimitiveID;
   // float r = (vertexColor & 0x000000FF) >> 0;
   // float g =  (vertexColor & 0x0000FF00) >> 8;
   // float b =  (vertexColor & 0x00FF0000) >> 16;
    //fColor = vec4(r, g, b, 1);

   // fColor = vec4(vertexColor/255, vertexColor/255, vertexColor/255, 1);

    //fColor = vec4((vertexColor & 0x000000FF) >> 0, (vertexColor & 0x0000FF00) >> 8, (vertexColor & 0x00FF0000) >> 16, (vertexColor & 0xFF000000)>> 24);

//    if(gl_PrimitiveID == 0)
//       fColor = vec4(1, 0, 0, 1);
//    else if (gl_PrimitiveID == 1)
//        fColor = vec4(0, 1, 0, 1);
//    else

    if (selection_mode == 0) {
        fColor = vec4(color, 0); // objects
    } else if (selection_mode == 1) {
        fColor = vec4(vertex_id/255.0f, 0, 0, 0); // polygons
    } else if (selection_mode == 2) {
        fColor = vec4((gl_PrimitiveID & 0xFF)/255.0f, ((gl_PrimitiveID >> 8) & 0xFF)/255.0f, ((gl_PrimitiveID >> 16) & 0xFF)/255.0f, 0);
    } else {
        fColor = vec4(255.0f, 255.0f, 255.0f, 0); // blank
    }

    //imageStore(select_image, gl_PrimitiveID, vec4(1,1,1,1));
}
