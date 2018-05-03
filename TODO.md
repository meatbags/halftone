colour support: 16/32bc

"float distanceToDotRadius( float channel, vec2 coord, vec2 normal, vec2 p, float angle, float rad_max ) {",

// apply shape-specific transforms
"float dist = hypot( coord.x - p.x, coord.y - p.y );",
"float rad = channel;",

"if ( shape == SHAPE_DOT ) {",

	"rad = pow( abs( rad ), 1.125 ) * rad_max;",

"} else if ( shape == SHAPE_ELLIPSE ) {",

	"rad = pow( abs( rad ), 1.125 ) * rad_max;",

	"if ( dist != 0.0 ) {",
		"float dot_p = abs( ( p.x - coord.x ) / dist * normal.x + ( p.y - coord.y ) / dist * normal.y );",
		"dist = ( dist * ( 1.0 - SQRT2_HALF_MINUS_ONE ) ) + dot_p * dist * SQRT2_MINUS_ONE;",
	"}",

"} else if ( shape == SHAPE_LINE ) {",

	"rad = pow( abs( rad ), 1.5) * rad_max;",
	"float dot_p = ( p.x - coord.x ) * normal.x + ( p.y - coord.y ) * normal.y;",
	"dist = hypot( normal.x * dot_p, normal.y * dot_p );",

"} else if ( shape == SHAPE_SQUARE ) {",

	"float theta = atan( p.y - coord.y, p.x - coord.x ) - angle;",
	"float sin_t = abs( sin( theta ) );",
	"float cos_t = abs( cos( theta ) );",
	"rad = pow( abs( rad ), 1.4 );",
	"rad = rad_max * ( rad + ( ( sin_t > cos_t ) ? rad - sin_t * rad : rad - cos_t * rad ) );",

"}",

"return rad - dist;",

"}",
