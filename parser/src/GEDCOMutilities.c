#include "GEDCOMutilities.h"
#include "GEDCOMparser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

char *errorType(GEDCOMerror err) {
	if (err.type == INV_FILE)
		return "INV_FILE";
	else if (err.type == INV_GEDCOM)
		return "INV_GEDCOM";
	else if (err.type == INV_HEADER)
		return "INV_HEADER";
	else if (err.type == INV_RECORD)
		return "INV_RECORD";
	else if (err.type == OTHER_ERROR)
		return "OTHER_ERROR";
	else if (err.type == WRITE_ERROR)
		return "WRITE_ERROR";
	else
		return "OK";
}

Field *addField(char *tag, char *value) {
	Field *field = malloc(sizeof(Field));
	field->tag = malloc(sizeof(char) * 32);
	field->value = malloc(sizeof(char) * 32);

	if (tag != NULL)
		strcpy(field->tag, tag);
	if (field != NULL)
		strcpy(field->value, value);

	return field;
}

Event *addEvent(char *type, char *date, char *place) {
	Event *event = malloc(sizeof(Event));
	event->date = malloc(sizeof(char) * 32);
	event->place = malloc(sizeof(char) * 32);
    event->otherFields = initializeList(&printField, &deleteDummy, &compareFields);

	strcpy(event->type, type);
	if (date != NULL) {
		strcpy(event->date, date);
	}
	if (place != NULL) {
		strcpy(event->place, place);
	}

	return event;
}

void addName(s_indiv *individual, char *name) {
	char *token;
	token = strtok(name, " ");

	if (name[0] == '/') {
		strcpy(individual->individual->givenName, "");
		token = strtok(name, "/");
		if (token != NULL)
			strcpy(individual->individual->surname, token);
		return;
	}
	else
		strcpy(individual->individual->givenName, token);

	token = strtok(NULL, " /");
	if (token != NULL)
		strcpy(individual->individual->surname, token);
}

Family *createFamily() {
	Family *family;
	family = malloc(sizeof(Family));
	family->wife = NULL;
	family->husband = NULL;
	family->events = initializeList(&printEvent, &deleteEvent, &compareDummy);
	family->children = initializeList(&printIndividual, &deleteDummy, &compareDummy);
    family->otherFields = initializeList(&printField, &deleteDummy, &compareDummy);

	return family;
}

Individual *createIndividual() {
    Individual *individual;
    individual = malloc(sizeof(Individual));
    individual->givenName = malloc(sizeof(char) * 128);
    individual->surname = malloc(sizeof(char) * 128);
    individual->events = initializeList(&printEvent, &deleteEvent, &compareEvents);
    individual->families = initializeList(&printFamily, &deleteDummy, &compareFamilies);
    individual->otherFields = initializeList(&printField, &deleteField, &compareFields);

    return individual;
}

CharSet convertCharSet(char *lineInfo) {
	if (strcmp(lineInfo, "ANSEL") == 0)
		return ANSEL;
	else if (strcmp(lineInfo, "UTF8") == 0 || strcmp(lineInfo, "UTF-8") == 0)
		return UTF8;
	else if (strcmp(lineInfo, "UNICODE") == 0)
		return UNICODE;
	else
		return ASCII;
}

void deleteDummy(void *toBeDeleted) {
}
char *printDummy(void *toBePrinted) {
	return NULL;
}
int compareDummy(const void *first, const void *second) {
	return 0;
}

bool compareFind(const void *first, const void *second) {
	if (first == second)
		return true;

	else if (first == NULL || second == NULL)
		return false;

	char *firstName = malloc(sizeof(char)*32);
	strcpy(firstName, ((Individual *)first)->givenName);
	strcat(firstName, ",");
	strcat(firstName, ((Individual *)first)->surname);

	char *secondName = malloc(sizeof(char)*32);
	strcpy(secondName, ((Individual *)second)->givenName);
	strcat(secondName, ",");
	strcat(secondName, ((Individual *)second)->surname);

	bool result;

	if (strcmp(firstName, secondName) == 0)
		result = true;
	else
		result = false;

	free(firstName);
	free(secondName);

	return result;
}

int getMonthValue(char *month) {
	if (strcmp(month, "JAN") == 0) {
		return 1;
	}
	else if (strcmp(month, "FEB") == 0) {
		return 2;
	}
	else if (strcmp(month, "MAR") == 0) {
		return 3;
	}
	else if (strcmp(month, "APR") == 0) {
		return 4;
	}
	else if (strcmp(month, "MAY") == 0) {
		return 5;
	}
	else if (strcmp(month, "JUN") == 0) {
		return 6;
	}
	else if (strcmp(month, "JUL") == 0) {
		return 7;
	}
	else if (strcmp(month, "AUG") == 0) {
		return 8;
	}
	else if (strcmp(month, "SEP") == 0) {
		return 9;
	}
	else if (strcmp(month, "OCT") == 0) {
		return 10;
	}
	else if (strcmp(month, "NOV") == 0) {
		return 11;
	}
	else if (strcmp(month, "DEC") == 0) {
		return 12;
	}
	else {
		printf("Invalid month\n");
		return -1;
	}
}

char *encodeTypeToStr(CharSet encoding) {
	if (encoding == ANSEL)
		return "ANSEL";
	else if (encoding == UTF8)
		return "UTF8";
	else if (encoding == UNICODE)
		return "UNICODE";
	else if (encoding == ASCII)
		return "ASCII";
	else
		return "Unknown character encoding type";
}

char *parseLine(char *token) {
	char *lineInfo = malloc(sizeof(char)*32);
	strcpy(lineInfo, "");
	while (token != NULL) {
//	while (token[strlen(token) - 1] != '\r' && token[strlen(token) - 1] != '\n') {
		token = strtok(NULL, " ");
		if (token != NULL) {
			if (token[strlen(token) - 1] != '\r' && token[strlen(token) - 1] != '\n') {
				strcat(lineInfo, token);
				strcat(lineInfo, " ");
			}
			else {
				token[strlen(token) - 1] = '\0';
				token = strtok(token, "\n\r");
				strcat(lineInfo, token);
			}
		}
		else break;
	}
	return lineInfo;
}

Submitter *createTestSubmitter(char *name, char *address) {
	Submitter *submitter = malloc(sizeof(Submitter) + strlen(address) + 1);

	submitter->otherFields = initializeList(&printField, &deleteField, &compareFields);

	strcpy(submitter->submitterName, name);
	strcpy(submitter->address, address);

	return submitter;
}

Header *createTestHeader(char *source, float gedcVersion, CharSet encoding, Submitter *headerSubmitter) {
	Header *header = malloc(sizeof(Header));

	header->otherFields = initializeList(&printField, &deleteField, &compareFields);

	strcpy(header->source, source);
	header->gedcVersion = gedcVersion;
	header->encoding = encoding;
	header->submitter = headerSubmitter;

	return header;
}

Event *createTestEvent(char *date, char *place, char *type) {
	Event *event = malloc(sizeof(Event));
	event->date = malloc(sizeof(char) * 128);
	event->place = malloc(sizeof(char) * 128);

	event->otherFields = initializeList(&printField, &deleteField, &compareFields);

	strcpy(event->date, date);
	strcpy(event->place, place);
	strcpy(event->type, type);

	return event;
}

Field *createTestField(char *tag, char *value) {
	Field *field = malloc(sizeof(Field));
	field->tag = malloc(sizeof(char) * 128);
	field->value = malloc(sizeof(char) * 128);

	strcpy(field->tag, tag);
	strcpy(field->value, value);

	return field;
}

Individual *createTestIndividual(char *givenName, char *surname) {
	Individual *individual = malloc(sizeof(Individual));
	individual->givenName = malloc(sizeof(char) * 128);
	individual->surname = malloc(sizeof(char) * 128);

	individual->events = initializeList(&printEvent, &deleteEvent, &compareEvents);
	individual->families = initializeList(&printFamily, &deleteDummy, &compareFamilies);
	individual->otherFields = initializeList(&printField, &deleteField, &compareFields);

	strcpy(individual->givenName, givenName);
	strcpy(individual->surname, surname);

	return individual;
}

Family *createTestFamily(Individual *husband, Individual *wife) {
	Family *family = malloc(sizeof(Family));
	family->husband = husband;
	family->wife = wife;

	family->events = initializeList(&printEvent, &deleteEvent, &compareEvents);
	family->children = initializeList(&printIndividual, &deleteDummy, &compareIndividuals);
	family->otherFields = initializeList(&printField, &deleteField, &compareFields);

	return family;
}

GEDCOMobject *createTestObject(Header *header, Submitter *submitter) {
	GEDCOMobject *object = malloc(sizeof(GEDCOMobject));
	object->header = header;
	object->submitter = submitter;

	object->families = initializeList(&printFamily, &deleteFamily, &compareFamilies);
	object->individuals = initializeList(&printIndividual, &deleteIndividual, &compareIndividuals);

	return object;
}

