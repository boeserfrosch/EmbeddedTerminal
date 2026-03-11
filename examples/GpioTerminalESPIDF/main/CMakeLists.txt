idf_component_register(SRCS "main.cpp"
                       INCLUDE_DIRS ".")

# GPIO Security Configuration
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    ET_GPIO_ENABLE=1
    ET_GPIO_ALLOWED_PINS="GPIO2,4,5,12,13,14,15"
    ET_GPIO_FORCED_EXCLUSIONS="GPIO0:r,w,m,e,i;GPIO45:r,w,m,i;GPIO46:r,w,m,i"
    ET_GPIO_DEFAULT_POLICY=1
    ET_GPIO_ADMIN_HASH="0xbf1075ac"
)
