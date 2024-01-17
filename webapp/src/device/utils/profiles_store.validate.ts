import Ajv from "ajv"
import { default as profileStoreSchema } from './profiles_store.schema.json'

const ajv = new Ajv()

//const schema: JSONSchemaType<ProfilesStoreData> = profileStoreSchema

// validate is a type guard for MyData - type is inferred from schema type
const validate = ajv.compile(profileStoreSchema)

export default validate
