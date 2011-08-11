The schema files is this directory are the transformed versions of those located in
    ../org.opendds.modeling.model/model/
    ../org.opendds.modeling.sdk.model/model/
    
They are used by the openddsvalidate tool and the eclipse plugin UI to validate 
.opendds and .codegen instance documents for XML validation errors.

If the opendds metamodel is changed in the plugins listed above, these xsd files must
be regenerated via tramsformation. To produc the transformation, execute the ant
build file in this plugin with the 'transform-xsd' target.