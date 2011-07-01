#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/XMLUri.hpp>
// these are for the parse function...
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/framework/Wrapper4InputSource.hpp>

#include "jalp_xml_utils.hpp"
#include "jalp_base64_internal.h"

XERCES_CPP_NAMESPACE_USE

const XMLCh JALP_XML_CORE[] = {
	chLatin_C, chLatin_o, chLatin_r, chLatin_e, chNull };


enum jal_status parse_xml_snippet(DOMElement *ctx_node, const char* snippet)
{
	Wrapper4InputSource *lsInput = NULL;
	MemBufInputSource * inputSource = NULL;
	DOMLSParser *parser = NULL;
	DOMImplementation *impl = NULL;
	DOMConfiguration *conf = NULL;

	if (!ctx_node) {
		return JAL_E_INVAL;
	}
	impl = DOMImplementationRegistry::getDOMImplementation(JALP_XML_CORE);
	parser = impl->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
	conf = parser->getDomConfig();
	conf->setParameter(XMLUni::fgDOMEntities, false);
	conf->setParameter(XMLUni::fgDOMNamespaces, true);
	// don't validate (can't since building a snippet
	conf->setParameter(XMLUni::fgDOMValidate, false);
	// Enable schema validation
	conf->setParameter(XMLUni::fgXercesSchema, false);
	// Enable schema validation
	conf->setParameter(XMLUni::fgXercesSchemaFullChecking, false);
	// Enable full checking
	conf->setParameter(XMLUni::fgXercesUseCachedGrammarInParse, false);
	// don't try and load unknown schemas
	conf->setParameter(XMLUni::fgXercesLoadSchema, false);
	// take ownership of the doc
	conf->setParameter(XMLUni::fgXercesUserAdoptsDOMDocument, true);

	inputSource = new MemBufInputSource(reinterpret_cast<const XMLByte*>(snippet),
					strlen(snippet),
					(char*)NULL,
					false);
	lsInput = new Wrapper4InputSource(inputSource);
	DOMNode *child_node = NULL;
	try {
		child_node = parser->parseWithContext(lsInput, ctx_node, DOMLSParser::ACTION_REPLACE_CHILDREN);
	} catch(...) {
		// do nothing
	}
	delete (parser);
	delete lsInput;
	return (child_node != NULL)? JAL_OK : JAL_E_XML_PARSE;
}

enum jal_status create_base64_element(DOMDocument *doc,
		const uint8_t *buffer,
		const size_t buf_len,
		const XMLCh *namespace_uri,
		const XMLCh *elm_name,
		DOMElement **new_elem)
{
	if (!doc || !buffer || (buf_len == 0) || !namespace_uri ||
		!elm_name || !new_elem || *new_elem) {
		return JAL_E_INVAL;
	}
	char *base64_val = NULL;
	XMLCh *xml_base64_val = NULL;

	base64_val = jalp_base64_enc(buffer, buf_len);
	if (!base64_val) {
		// this should never actually happen since the input is
		// non-zero in length.
		return JAL_E_INVAL;
	}

	xml_base64_val = XMLString::transcode(base64_val);

	DOMElement *elm = doc->createElementNS(namespace_uri, elm_name);
	elm->setTextContent(xml_base64_val);

	XMLString::release(&xml_base64_val);
	free(base64_val);
	*new_elem = elm;
	return JAL_OK;
}